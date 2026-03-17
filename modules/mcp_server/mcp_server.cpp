/**************************************************************************/
/*  mcp_server.cpp                                                        */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#include "mcp_server.h"

#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/io/json.h"
#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "core/object/callable_mp.h"
#include "core/object/class_db.h"
#include "core/object/script_language.h"
#include "core/os/os.h"
#include "core/variant/typed_array.h"
#include "core/version.h"
#include "scene/main/node.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"
#include "scene/resources/packed_scene.h"

// Tool registration includes.
#include "tools/mcp_tools_3d_physics.h"
#include "tools/mcp_tools_3d_scene.h"
#include "tools/mcp_tools_3d_transform.h"
#include "tools/mcp_tools_animation.h"
#include "tools/mcp_tools_api_docs.h"
#include "tools/mcp_tools_assets.h"
#include "tools/mcp_tools_batch.h"
#include "tools/mcp_tools_codegen.h"
#include "tools/mcp_tools_core.h"
#include "tools/mcp_tools_events.h"
#include "tools/mcp_tools_linter.h"
#include "tools/mcp_tools_materials.h"
#include "tools/mcp_tools_mesh_gen.h"
#include "tools/mcp_tools_navigation.h"
#include "tools/mcp_tools_particles.h"
#include "tools/mcp_tools_runtime.h"
#include "tools/mcp_tools_screenshot.h"
#include "tools/mcp_tools_script_editing.h"
#include "tools/mcp_tools_snapshot.h"
#include "tools/mcp_tools_templates.h"
#include "tools/mcp_tools_terrain.h"
#include "tools/mcp_tools_tilemap.h"
#include "tools/mcp_tools_hot_reload.h"
#include "tools/mcp_tools_testing.h"

#ifndef _WIN32
#include <fcntl.h>
#include <unistd.h>
#else
#include <io.h>
#include <windows.h>
#endif

MCPServer *MCPServer::singleton = nullptr;

MCPServer::MCPServer() {
	singleton = this;
	_register_builtin_tools();
	_register_builtin_resources();
	_register_builtin_prompts();
}

MCPServer::~MCPServer() {
	stop();
	singleton = nullptr;
}

void MCPServer::_bind_methods() {
	ClassDB::bind_method(D_METHOD("start_websocket", "port", "bind_host"), &MCPServer::start_websocket, DEFVAL("127.0.0.1"));
	ClassDB::bind_method(D_METHOD("start_stdio"), &MCPServer::start_stdio);
	ClassDB::bind_method(D_METHOD("stop"), &MCPServer::stop);
	ClassDB::bind_method(D_METHOD("poll", "limit_usec"), &MCPServer::poll, DEFVAL(0));
	ClassDB::bind_method(D_METHOD("is_running"), &MCPServer::is_running);
	ClassDB::bind_method(D_METHOD("get_transport_mode"), &MCPServer::get_transport_mode);
	ClassDB::bind_method(D_METHOD("register_tool", "name", "description", "input_schema", "handler"), &MCPServer::register_tool);
	ClassDB::bind_method(D_METHOD("unregister_tool", "name"), &MCPServer::unregister_tool);
	ClassDB::bind_method(D_METHOD("get_tool_list"), &MCPServer::get_tool_list);
	ClassDB::bind_method(D_METHOD("send_notification", "event_type", "data", "client_id"), &MCPServer::send_notification, DEFVAL(-1));
	ClassDB::bind_method(D_METHOD("broadcast_notification", "event_type", "data"), &MCPServer::broadcast_notification);

	BIND_ENUM_CONSTANT(TRANSPORT_NONE);
	BIND_ENUM_CONSTANT(TRANSPORT_STDIO);
	BIND_ENUM_CONSTANT(TRANSPORT_WEBSOCKET);
}

// =============================================================================
// Transport
// =============================================================================

Error MCPServer::start_websocket(int p_port, const String &p_bind_host) {
	if (running) {
		stop();
	}

	tcp_server.instantiate();
	Error err = tcp_server->listen(p_port, p_bind_host);
	if (err != OK) {
		tcp_server.unref();
		ERR_FAIL_V_MSG(err, "MCPServer: Failed to listen on port " + itos(p_port));
	}

	transport_mode = TRANSPORT_WEBSOCKET;
	running = true;
	print_line("MCPServer: Listening on " + p_bind_host + ":" + itos(p_port));
	return OK;
}

Error MCPServer::start_stdio() {
	if (running) {
		stop();
	}

#ifndef _WIN32
	int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
	fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
#endif

	transport_mode = TRANSPORT_STDIO;
	running = true;
	return OK;
}

void MCPServer::stop() {
	if (!running) {
		return;
	}

	if (transport_mode == TRANSPORT_WEBSOCKET) {
		for (KeyValue<int, WSClient> &kv : ws_clients) {
			kv.value.ws_peer->close();
		}
		ws_clients.clear();
		if (tcp_server.is_valid()) {
			tcp_server->stop();
			tcp_server.unref();
		}
	}

	transport_mode = TRANSPORT_NONE;
	running = false;
	initialized = false;
	stdin_buffer = "";
	event_subscriptions.clear();
}

void MCPServer::poll(int p_limit_usec) {
	if (!running) {
		return;
	}

	switch (transport_mode) {
		case TRANSPORT_WEBSOCKET:
			_poll_websocket();
			break;
		case TRANSPORT_STDIO:
			_poll_stdio();
			break;
		default:
			break;
	}
}

// =============================================================================
// WebSocket Transport
// =============================================================================

void MCPServer::_poll_websocket() {
	MutexLock lock(mutex);
	while (tcp_server.is_valid() && tcp_server->is_connection_available()) {
		Ref<StreamPeerTCP> tcp_peer = tcp_server->take_connection();
		if (tcp_peer.is_valid()) {
			Ref<WebSocketPeer> ws_peer = Ref<WebSocketPeer>(WebSocketPeer::create());
			ERR_CONTINUE(ws_peer.is_null());
			ws_peer->accept_stream(tcp_peer);

			int client_id = next_client_id++;
			WSClient client;
			client.ws_peer = ws_peer;
			client.ws_connected = false;
			ws_clients[client_id] = client;
		}
	}

	Vector<int> to_remove;
	for (KeyValue<int, WSClient> &kv : ws_clients) {
		int id = kv.key;
		WSClient &client = kv.value;
		client.ws_peer->poll();

		WebSocketPeer::State state = client.ws_peer->get_ready_state();

		if (state == WebSocketPeer::STATE_OPEN) {
			client.ws_connected = true;
			while (client.ws_peer->get_available_packet_count() > 0) {
				const uint8_t *data;
				int data_size;
				Error err = client.ws_peer->get_packet(&data, data_size);
				if (err == OK) {
					String message = String::utf8((const char *)data, data_size);
					_process_message(message, id);
				}
			}
		} else if (state == WebSocketPeer::STATE_CLOSED) {
			to_remove.push_back(id);
		}
	}

	for (int id : to_remove) {
		// Clean up subscriptions for disconnected client.
		for (KeyValue<String, HashSet<int>> &kv : event_subscriptions) {
			kv.value.erase(id);
		}
		ws_clients.erase(id);
	}
}

// =============================================================================
// stdio Transport
// =============================================================================

void MCPServer::_poll_stdio() {
#ifndef _WIN32
	static const int MAX_STDIN_BUFFER = 100 * 1024 * 1024; // 100 MB.
	char buf[4096];
	ssize_t n = read(STDIN_FILENO, buf, sizeof(buf));
	if (n > 0) {
		if (stdin_buffer.length() + n > MAX_STDIN_BUFFER) {
			ERR_PRINT("MCP stdio buffer exceeded 100MB limit, clearing.");
			stdin_buffer = String();
			return;
		}
		stdin_buffer += String::utf8(buf, n);
	}
#else
	HANDLE h = GetStdHandle(STD_INPUT_HANDLE);
	DWORD avail = 0;
	if (PeekNamedPipe(h, nullptr, 0, nullptr, &avail, nullptr) && avail > 0) {
		char *buf = (char *)memalloc(avail + 1);
		DWORD read_bytes = 0;
		ReadFile(h, buf, avail, &read_bytes, nullptr);
		if (read_bytes > 0) {
			stdin_buffer += String::utf8(buf, read_bytes);
		}
		memfree(buf);
	}
#endif

	String message;
	while (_try_parse_stdio_message(message)) {
		_process_message(message, -1);
	}
}

bool MCPServer::_try_parse_stdio_message(String &r_message) {
	int header_end = stdin_buffer.find("\r\n\r\n");
	if (header_end == -1) {
		return false;
	}

	String header_section = stdin_buffer.substr(0, header_end);
	int content_length = -1;

	Vector<String> headers = header_section.split("\r\n");
	for (const String &h : headers) {
		if (h.begins_with("Content-Length:")) {
			content_length = h.substr(String("Content-Length:").length()).strip_edges().to_int();
			break;
		}
	}

	if (content_length < 0) {
		stdin_buffer = stdin_buffer.substr(header_end + 4);
		return false;
	}

	int body_start = header_end + 4;
	int total_needed = body_start + content_length;

	if (stdin_buffer.length() < total_needed) {
		return false;
	}

	r_message = stdin_buffer.substr(body_start, content_length);
	stdin_buffer = stdin_buffer.substr(total_needed);
	return true;
}

String MCPServer::_frame_message(const String &p_json) const {
	CharString utf8 = p_json.utf8();
	return "Content-Length: " + itos(utf8.length()) + "\r\n\r\n" + p_json;
}

void MCPServer::_send_response(const Dictionary &p_response, int p_client_id) {
	String json = JSON::stringify(p_response);

	if (transport_mode == TRANSPORT_STDIO) {
		_send_stdio_message(json);
	} else if (transport_mode == TRANSPORT_WEBSOCKET) {
		if (p_client_id >= 0) {
			_send_ws_message(json, p_client_id);
		} else {
			for (KeyValue<int, WSClient> &kv : ws_clients) {
				if (kv.value.ws_connected) {
					_send_ws_message(json, kv.key);
				}
			}
		}
	}
}

void MCPServer::_send_stdio_message(const String &p_json) {
	String framed = _frame_message(p_json);
	CharString utf8 = framed.utf8();
#ifndef _WIN32
	::write(STDOUT_FILENO, utf8.get_data(), utf8.length());
#else
	HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD written = 0;
	WriteFile(h, utf8.get_data(), utf8.length(), &written, nullptr);
#endif
}

void MCPServer::_send_ws_message(const String &p_json, int p_client_id) {
	if (ws_clients.has(p_client_id)) {
		CharString utf8 = p_json.utf8();
		ws_clients[p_client_id].ws_peer->send((const uint8_t *)utf8.get_data(), utf8.length(), WebSocketPeer::WRITE_MODE_TEXT);
	}
}

// =============================================================================
// Message Processing
// =============================================================================

void MCPServer::_process_message(const String &p_message, int p_client_id) {
	Ref<JSON> json;
	json.instantiate();
	Error err = json->parse(p_message);
	if (err != OK) {
		Dictionary error_resp;
		error_resp["jsonrpc"] = "2.0";
		error_resp["id"] = Variant();
		Dictionary error_obj;
		error_obj["code"] = -32700;
		error_obj["message"] = "Parse error: " + json->get_error_message();
		error_resp["error"] = error_obj;
		_send_response(error_resp, p_client_id);
		return;
	}
	Variant parsed = json->get_data();

	if (parsed.get_type() != Variant::DICTIONARY) {
		return;
	}

	Dictionary msg = parsed;
	String method = msg.get("method", "");
	Variant id = msg.get("id", Variant());
	Dictionary params = msg.get("params", Dictionary());

	Dictionary response;

	// MCP protocol methods.
	if (method == "initialize") {
		response = _handle_initialize(params, id);
	} else if (method == "notifications/initialized" || method == "initialized") {
		response = _handle_initialized_notification();
	} else if (method == "ping") {
		response = _handle_ping(id);
	}
	// Tools.
	else if (method == "tools/list") {
		response = _handle_tools_list(id);
	} else if (method == "tools/call") {
		response = _handle_tools_call(params, id);
	}
	// Resources.
	else if (method == "resources/list") {
		response = _handle_resources_list(id);
	} else if (method == "resources/read") {
		response = _handle_resources_read(params, id);
	}
	// Prompts.
	else if (method == "prompts/list") {
		response = _handle_prompts_list(id);
	} else if (method == "prompts/get") {
		response = _handle_prompts_get(params, id);
	}
	// Unknown.
	else {
		if (id.get_type() != Variant::NIL) {
			response["jsonrpc"] = "2.0";
			response["id"] = id;
			Dictionary error_obj;
			error_obj["code"] = -32601;
			error_obj["message"] = "Method not found: " + method;
			response["error"] = error_obj;
		}
	}

	if (!response.is_empty()) {
		_send_response(response, p_client_id);
	}
}

// =============================================================================
// MCP Protocol Handlers
// =============================================================================

Dictionary MCPServer::_handle_initialize(const Dictionary &p_params, const Variant &p_id) {
	Dictionary response;
	response["jsonrpc"] = "2.0";
	response["id"] = p_id;

	Dictionary result;
	result["protocolVersion"] = "2024-11-05";

	Dictionary server_info;
	server_info["name"] = server_name;
	server_info["version"] = server_version;
	result["serverInfo"] = server_info;

	Dictionary capabilities;

	// Tools capability.
	Dictionary tools_cap;
	tools_cap["listChanged"] = false;
	capabilities["tools"] = tools_cap;

	// Resources capability.
	Dictionary resources_cap;
	resources_cap["subscribe"] = false;
	resources_cap["listChanged"] = false;
	capabilities["resources"] = resources_cap;

	// Prompts capability.
	Dictionary prompts_cap;
	prompts_cap["listChanged"] = false;
	capabilities["prompts"] = prompts_cap;

	result["capabilities"] = capabilities;
	response["result"] = result;

	return response;
}

Dictionary MCPServer::_handle_initialized_notification() {
	initialized = true;
	return Dictionary();
}

Dictionary MCPServer::_handle_ping(const Variant &p_id) {
	Dictionary response;
	response["jsonrpc"] = "2.0";
	response["id"] = p_id;
	response["result"] = Dictionary();
	return response;
}

// =============================================================================
// Tools Protocol
// =============================================================================

Dictionary MCPServer::_handle_tools_list(const Variant &p_id) {
	Dictionary response;
	response["jsonrpc"] = "2.0";
	response["id"] = p_id;

	Dictionary result;
	Array tools;

	for (const KeyValue<String, MCPToolDef> &kv : mcp_tools) {
		Dictionary tool;
		tool["name"] = kv.value.name;
		tool["description"] = kv.value.description;
		tool["inputSchema"] = kv.value.input_schema;
		tools.push_back(tool);
	}

	result["tools"] = tools;
	response["result"] = result;
	return response;
}

Dictionary MCPServer::_handle_tools_call(const Dictionary &p_params, const Variant &p_id) {
	Dictionary response;
	response["jsonrpc"] = "2.0";
	response["id"] = p_id;

	String tool_name = p_params.get("name", "");
	Dictionary tool_args = p_params.get("arguments", Dictionary());

	if (!mcp_tools.has(tool_name)) {
		Dictionary error_obj;
		error_obj["code"] = -32602;
		error_obj["message"] = "Unknown tool: " + tool_name;
		response["error"] = error_obj;
		return response;
	}

	const MCPToolDef &tool = mcp_tools[tool_name];

	Dictionary tool_result;
	if (tool.handler.is_valid()) {
		Variant ret;
		Variant args_var = tool_args;
		const Variant *argv[1] = { &args_var };
		Callable::CallError call_err;
		tool.handler.callp(argv, 1, ret, call_err);
		if (call_err.error != Callable::CallError::CALL_OK) {
			Dictionary error_obj;
			error_obj["code"] = -32603;
			error_obj["message"] = "Tool execution error for: " + tool_name;
			response["error"] = error_obj;
			return response;
		}
		tool_result = ret;
	}

	Dictionary result;
	Array content;
	Dictionary text_content;
	text_content["type"] = "text";
	text_content["text"] = JSON::stringify(tool_result);
	content.push_back(text_content);
	result["content"] = content;
	response["result"] = result;
	return response;
}

// =============================================================================
// Resources Protocol
// =============================================================================

Dictionary MCPServer::_handle_resources_list(const Variant &p_id) {
	Dictionary response;
	response["jsonrpc"] = "2.0";
	response["id"] = p_id;

	Dictionary result;
	Array resources;

	for (const KeyValue<String, MCPResourceDef> &kv : mcp_resources) {
		Dictionary resource;
		resource["uri"] = kv.value.uri;
		resource["name"] = kv.value.name;
		resource["description"] = kv.value.description;
		if (!kv.value.mime_type.is_empty()) {
			resource["mimeType"] = kv.value.mime_type;
		}
		resources.push_back(resource);
	}

	result["resources"] = resources;
	response["result"] = result;
	return response;
}

Dictionary MCPServer::_handle_resources_read(const Dictionary &p_params, const Variant &p_id) {
	Dictionary response;
	response["jsonrpc"] = "2.0";
	response["id"] = p_id;

	String uri = p_params.get("uri", "");

	if (!mcp_resources.has(uri)) {
		Dictionary error_obj;
		error_obj["code"] = -32602;
		error_obj["message"] = "Unknown resource: " + uri;
		response["error"] = error_obj;
		return response;
	}

	const MCPResourceDef &resource = mcp_resources[uri];
	Dictionary resource_result;

	if (resource.handler.is_valid()) {
		Variant ret;
		Variant uri_var = uri;
		const Variant *argv[1] = { &uri_var };
		Callable::CallError call_err;
		resource.handler.callp(argv, 1, ret, call_err);
		if (call_err.error != Callable::CallError::CALL_OK) {
			Dictionary error_obj;
			error_obj["code"] = -32603;
			error_obj["message"] = "Resource handler execution error for: " + uri;
			response["error"] = error_obj;
			return response;
		}
		resource_result = ret;
	}

	response["result"] = resource_result;
	return response;
}

// =============================================================================
// Prompts Protocol
// =============================================================================

Dictionary MCPServer::_handle_prompts_list(const Variant &p_id) {
	Dictionary response;
	response["jsonrpc"] = "2.0";
	response["id"] = p_id;

	Dictionary result;
	Array prompts;

	for (const KeyValue<String, MCPPromptDef> &kv : mcp_prompts) {
		Dictionary prompt;
		prompt["name"] = kv.value.name;
		prompt["description"] = kv.value.description;
		if (!kv.value.arguments.is_empty()) {
			prompt["arguments"] = kv.value.arguments;
		}
		prompts.push_back(prompt);
	}

	result["prompts"] = prompts;
	response["result"] = result;
	return response;
}

Dictionary MCPServer::_handle_prompts_get(const Dictionary &p_params, const Variant &p_id) {
	Dictionary response;
	response["jsonrpc"] = "2.0";
	response["id"] = p_id;

	String prompt_name = p_params.get("name", "");
	Dictionary prompt_args = p_params.get("arguments", Dictionary());

	if (!mcp_prompts.has(prompt_name)) {
		Dictionary error_obj;
		error_obj["code"] = -32602;
		error_obj["message"] = "Unknown prompt: " + prompt_name;
		response["error"] = error_obj;
		return response;
	}

	const MCPPromptDef &prompt = mcp_prompts[prompt_name];
	Dictionary prompt_result;

	if (prompt.handler.is_valid()) {
		Variant ret;
		Variant args_var = prompt_args;
		const Variant *argv[1] = { &args_var };
		Callable::CallError call_err;
		prompt.handler.callp(argv, 1, ret, call_err);
		if (call_err.error != Callable::CallError::CALL_OK) {
			Dictionary error_obj;
			error_obj["code"] = -32603;
			error_obj["message"] = "Prompt handler execution error for: " + prompt_name;
			response["error"] = error_obj;
			return response;
		}
		prompt_result = ret;
	}

	response["result"] = prompt_result;
	return response;
}

// =============================================================================
// Registration APIs
// =============================================================================

void MCPServer::register_tool(const String &p_name, const String &p_description, const Dictionary &p_input_schema, const Callable &p_handler) {
	ERR_FAIL_COND_MSG(p_name.is_empty(), "Tool name cannot be empty.");
	ERR_FAIL_COND_MSG(!p_handler.is_valid(), vformat("Tool handler for '%s' is not valid.", p_name));
	MCPToolDef tool;
	tool.name = p_name;
	tool.description = p_description;
	tool.input_schema = p_input_schema;
	tool.handler = p_handler;
	mcp_tools[p_name] = tool;
}

void MCPServer::unregister_tool(const String &p_name) {
	mcp_tools.erase(p_name);
}

TypedArray<Dictionary> MCPServer::get_tool_list() const {
	TypedArray<Dictionary> result;
	for (const KeyValue<String, MCPToolDef> &kv : mcp_tools) {
		Dictionary tool;
		tool["name"] = kv.value.name;
		tool["description"] = kv.value.description;
		tool["inputSchema"] = kv.value.input_schema;
		result.push_back(tool);
	}
	return result;
}

void MCPServer::register_resource(const String &p_uri, const String &p_name, const String &p_description, const String &p_mime_type, const Callable &p_handler) {
	ERR_FAIL_COND_MSG(p_uri.is_empty(), "Resource URI cannot be empty.");
	ERR_FAIL_COND_MSG(!p_handler.is_valid(), vformat("Resource handler for '%s' is not valid.", p_uri));
	MCPResourceDef resource;
	resource.uri = p_uri;
	resource.name = p_name;
	resource.description = p_description;
	resource.mime_type = p_mime_type;
	resource.handler = p_handler;
	mcp_resources[p_uri] = resource;
}

void MCPServer::unregister_resource(const String &p_uri) {
	mcp_resources.erase(p_uri);
}

void MCPServer::register_prompt(const String &p_name, const String &p_description, const Array &p_arguments, const Callable &p_handler) {
	ERR_FAIL_COND_MSG(p_name.is_empty(), "Prompt name cannot be empty.");
	ERR_FAIL_COND_MSG(!p_handler.is_valid(), vformat("Prompt handler for '%s' is not valid.", p_name));
	MCPPromptDef prompt;
	prompt.name = p_name;
	prompt.description = p_description;
	prompt.arguments = p_arguments;
	prompt.handler = p_handler;
	mcp_prompts[p_name] = prompt;
}

void MCPServer::unregister_prompt(const String &p_name) {
	mcp_prompts.erase(p_name);
}

// =============================================================================
// Event Push
// =============================================================================

void MCPServer::send_notification(const String &p_event_type, const Dictionary &p_data, int p_client_id) {
	Dictionary notification;
	notification["jsonrpc"] = "2.0";
	notification["method"] = "notifications/" + p_event_type;
	notification["params"] = p_data;
	_send_response(notification, p_client_id);
}

void MCPServer::broadcast_notification(const String &p_event_type, const Dictionary &p_data) {
	if (event_subscriptions.has(p_event_type)) {
		Dictionary notification;
		notification["jsonrpc"] = "2.0";
		notification["method"] = "notifications/" + p_event_type;
		notification["params"] = p_data;

		for (int client_id : event_subscriptions[p_event_type]) {
			_send_response(notification, client_id);
		}
	}
}

// =============================================================================
// Built-in Registration
// =============================================================================

void MCPServer::_register_builtin_tools() {
	// Core tools (ping, engine info, project, scene, script, editor).
	mcp_register_core_tools(this);

	// Diff-based script editing.
	mcp_register_script_editing_tools(this);

	// 3D scene creation.
	mcp_register_3d_scene_tools(this);

	// Materials and shaders.
	mcp_register_materials_tools(this);

	// Primitive mesh generation.
	mcp_register_mesh_gen_tools(this);

	// 3D transforms.
	mcp_register_3d_transform_tools(this);

	// 3D physics.
	mcp_register_3d_physics_tools(this);

	// Navigation.
	mcp_register_navigation_tools(this);

	// Particles.
	mcp_register_particles_tools(this);

	// Terrain and environment.
	mcp_register_terrain_tools(this);

	// Runtime state queries.
	mcp_register_runtime_tools(this);

	// Screenshot and visual.
	mcp_register_screenshot_tools(this);

	// Scene snapshots.
	mcp_register_snapshot_tools(this);

	// Batch operations.
	mcp_register_batch_tools(this);

	// Event subscriptions.
	mcp_register_events_tools(this);

	// Animation.
	mcp_register_animation_tools(this);

	// Assets.
	mcp_register_assets_tools(this);

	// GDScript linter.
	mcp_register_linter_tools(this);

	// Code generation.
	mcp_register_codegen_tools(this);

	// API documentation.
	mcp_register_api_docs_tools(this);

	// TileMap.
	mcp_register_tilemap_tools(this);

	// Project templates.
	mcp_register_templates_tools(this);

	// Hot reload.
	mcp_register_hot_reload_tools(this);

	// Testing.
	mcp_register_testing_tools(this);
}

// =============================================================================
// Built-in Resources
// =============================================================================

static Dictionary _resource_scene_current(const String &p_uri) {
	Dictionary result;
	Array contents;

	SceneTree *st = SceneTree::get_singleton();
	if (st && st->get_root()) {
		// Build scene tree as text.
		struct NodeDumper {
			static String dump(Node *p_node, int p_indent) {
				String indent_str;
				for (int i = 0; i < p_indent; i++) {
					indent_str += "  ";
				}
				String line = indent_str + "- " + p_node->get_name() + " (" + p_node->get_class() + ")";
				Ref<Script> script = p_node->get_script();
				if (script.is_valid()) {
					line += " [" + script->get_path() + "]";
				}
				line += "\n";
				for (int i = 0; i < p_node->get_child_count(); i++) {
					line += dump(p_node->get_child(i), p_indent + 1);
				}
				return line;
			}
		};

		Dictionary content;
		content["uri"] = p_uri;
		content["mimeType"] = "text/plain";
		content["text"] = NodeDumper::dump(st->get_root(), 0);
		contents.push_back(content);
	}

	result["contents"] = contents;
	return result;
}

static Dictionary _resource_project_settings(const String &p_uri) {
	Dictionary result;
	Array contents;

	ProjectSettings *ps = ProjectSettings::get_singleton();
	if (ps) {
		Dictionary settings;
		settings["application/config/name"] = ps->get_setting("application/config/name", "");
		settings["application/run/main_scene"] = ps->get_setting("application/run/main_scene", "");
		settings["display/window/size/viewport_width"] = ps->get_setting("display/window/size/viewport_width", 1152);
		settings["display/window/size/viewport_height"] = ps->get_setting("display/window/size/viewport_height", 648);
		settings["rendering/renderer/rendering_method"] = ps->get_setting("rendering/renderer/rendering_method", "forward_plus");

		Dictionary content;
		content["uri"] = p_uri;
		content["mimeType"] = "application/json";
		content["text"] = JSON::stringify(settings, "\t");
		contents.push_back(content);
	}

	result["contents"] = contents;
	return result;
}

static Dictionary _resource_logs_recent(const String &p_uri) {
	Dictionary result;
	Array contents;

	Dictionary content;
	content["uri"] = p_uri;
	content["mimeType"] = "text/plain";
	content["text"] = "Recent logs are streamed via --structured-output. Use the structured logger for real-time log capture.";
	contents.push_back(content);

	result["contents"] = contents;
	return result;
}

void MCPServer::_register_builtin_resources() {
	register_resource("godot://scene/current", "Current Scene Tree",
			"The current scene tree hierarchy with node types and scripts",
			"text/plain", callable_mp_static(&_resource_scene_current));

	register_resource("godot://project/settings", "Project Settings",
			"Key project settings (name, main scene, viewport size, renderer)",
			"application/json", callable_mp_static(&_resource_project_settings));

	register_resource("godot://logs/recent", "Recent Logs",
			"Recent engine log output",
			"text/plain", callable_mp_static(&_resource_logs_recent));
}

// =============================================================================
// Built-in Prompts
// =============================================================================

static Dictionary _prompt_debug_scene(const Dictionary &p_args) {
	Dictionary result;
	Array messages;

	String scene_info = "Analyze the current Godot scene for potential issues:\n\n";
	scene_info += "1. Check for orphaned nodes (nodes without proper ownership)\n";
	scene_info += "2. Check for nodes with missing scripts\n";
	scene_info += "3. Check for deeply nested node hierarchies (>10 levels)\n";
	scene_info += "4. Check for nodes with default names (suggesting they haven't been properly named)\n";
	scene_info += "5. Check for collision shapes without physics bodies\n";
	scene_info += "6. Suggest optimizations for the scene structure\n\n";

	SceneTree *st = SceneTree::get_singleton();
	if (st && st->get_root()) {
		scene_info += "Current scene tree:\n";
		struct NodeLister {
			static void list(Node *p_node, int p_indent, String &r_text) {
				for (int i = 0; i < p_indent; i++) {
					r_text += "  ";
				}
				r_text += "- " + p_node->get_name() + " (" + p_node->get_class() + ")\n";
				for (int i = 0; i < p_node->get_child_count(); i++) {
					list(p_node->get_child(i), p_indent + 1, r_text);
				}
			}
		};
		NodeLister::list(st->get_root(), 0, scene_info);
	}

	Dictionary msg;
	msg["role"] = "user";
	Dictionary content;
	content["type"] = "text";
	content["text"] = scene_info;
	msg["content"] = content;
	messages.push_back(msg);

	result["messages"] = messages;
	return result;
}

static Dictionary _prompt_fix_script_errors(const Dictionary &p_args) {
	Dictionary result;
	Array messages;

	String prompt_text = "Review all GDScript files in this project for errors:\n\n";
	prompt_text += "1. Use the `lint_project` tool to get all linting diagnostics\n";
	prompt_text += "2. Use the `validate_script` tool on each .gd file\n";
	prompt_text += "3. For each error found, provide a fix using `patch_script`\n";
	prompt_text += "4. After fixing, re-validate to confirm the fix\n\n";
	prompt_text += "Focus on: type annotation issues, unused variables, missing return types, and syntax errors.";

	Dictionary msg;
	msg["role"] = "user";
	Dictionary content;
	content["type"] = "text";
	content["text"] = prompt_text;
	msg["content"] = content;
	messages.push_back(msg);

	result["messages"] = messages;
	return result;
}

static Dictionary _prompt_optimize_scene(const Dictionary &p_args) {
	Dictionary result;
	Array messages;

	String prompt_text = "Optimize the current Godot scene for better performance:\n\n";
	prompt_text += "1. Use `get_performance_metrics` to check current FPS and draw calls\n";
	prompt_text += "2. Use `get_scene_tree` to analyze the node hierarchy\n";
	prompt_text += "3. Look for:\n";
	prompt_text += "   - Too many individual nodes that could be merged\n";
	prompt_text += "   - Missing visibility culling (VisibleOnScreenNotifier3D)\n";
	prompt_text += "   - Inefficient physics setups\n";
	prompt_text += "   - Overdraw issues with overlapping transparent materials\n";
	prompt_text += "   - LOD (Level of Detail) opportunities for 3D meshes\n";
	prompt_text += "4. Suggest specific changes and implement them using MCP tools";

	Dictionary msg;
	msg["role"] = "user";
	Dictionary content;
	content["type"] = "text";
	content["text"] = prompt_text;
	msg["content"] = content;
	messages.push_back(msg);

	result["messages"] = messages;
	return result;
}

static Dictionary _prompt_review_project(const Dictionary &p_args) {
	Dictionary result;
	Array messages;

	String prompt_text = "Perform a comprehensive project health check:\n\n";
	prompt_text += "1. Use `get_project_structure` to review file organization\n";
	prompt_text += "2. Use `lint_project` to check all scripts\n";
	prompt_text += "3. Use `get_project_settings` to verify configuration\n";
	prompt_text += "4. Check for:\n";
	prompt_text += "   - Files in wrong directories\n";
	prompt_text += "   - Unused scripts or scenes\n";
	prompt_text += "   - Missing or broken scene references\n";
	prompt_text += "   - Code quality issues across all scripts\n";
	prompt_text += "   - Missing type annotations\n";
	prompt_text += "   - Inconsistent naming conventions\n";
	prompt_text += "5. Provide a summary report with priority-ordered recommendations";

	Dictionary msg;
	msg["role"] = "user";
	Dictionary content;
	content["type"] = "text";
	content["text"] = prompt_text;
	msg["content"] = content;
	messages.push_back(msg);

	result["messages"] = messages;
	return result;
}

static Dictionary _prompt_create_3d_game(const Dictionary &p_args) {
	Dictionary result;
	Array messages;

	String game_type = p_args.get("game_type", "third_person");

	String prompt_text = "Create a " + game_type + " 3D game from scratch using MCP tools:\n\n";
	prompt_text += "1. Use `setup_3d_environment` to create the world (lighting, sky)\n";
	prompt_text += "2. Create ground plane with `create_mesh_instance` + `create_static_body`\n";
	prompt_text += "3. Create a player with `create_character_body_3d` + `create_mesh_instance`\n";
	prompt_text += "4. Write player movement script with `create_script`\n";
	prompt_text += "5. Attach script with `attach_script`\n";
	prompt_text += "6. Add a camera with `create_camera_3d`\n";
	prompt_text += "7. Add obstacles/environment with physics bodies\n";
	prompt_text += "8. Set up navigation if needed\n";
	prompt_text += "9. Save the scene with `save_scene`\n";
	prompt_text += "10. Test by running with `run_project`\n\n";
	prompt_text += "Use proper type annotations in all GDScript code.";

	Dictionary msg;
	msg["role"] = "user";
	Dictionary content;
	content["type"] = "text";
	content["text"] = prompt_text;
	msg["content"] = content;
	messages.push_back(msg);

	result["messages"] = messages;
	return result;
}

void MCPServer::_register_builtin_prompts() {
	{
		Array args;
		register_prompt("debug_scene", "Analyze the current scene for issues and suggest fixes", args,
				callable_mp_static(&_prompt_debug_scene));
	}

	{
		Array args;
		register_prompt("fix_script_errors", "Find and fix all GDScript errors in the project", args,
				callable_mp_static(&_prompt_fix_script_errors));
	}

	{
		Array args;
		register_prompt("optimize_scene", "Analyze and optimize the current scene for performance", args,
				callable_mp_static(&_prompt_optimize_scene));
	}

	{
		Array args;
		register_prompt("review_project", "Perform a comprehensive project health check", args,
				callable_mp_static(&_prompt_review_project));
	}

	{
		Array args;
		Dictionary game_type_arg;
		game_type_arg["name"] = "game_type";
		game_type_arg["description"] = "Type of 3D game: first_person, third_person, top_down, platformer";
		game_type_arg["required"] = false;
		args.push_back(game_type_arg);
		register_prompt("create_3d_game", "Step-by-step guide to create a 3D game using MCP tools", args,
				callable_mp_static(&_prompt_create_3d_game));
	}
}
