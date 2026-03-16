/**************************************************************************/
/*  mcp_server.h                                                          */
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

#pragma once

#include "core/io/json.h"
#include "core/io/tcp_server.h"
#include "core/os/mutex.h"
#include "core/templates/hash_set.h"
#include "modules/jsonrpc/jsonrpc.h"
#include "modules/websocket/websocket_peer.h"

class MCPServer : public Object {
	GDCLASS(MCPServer, Object)

public:
	enum TransportMode {
		TRANSPORT_NONE,
		TRANSPORT_STDIO,
		TRANSPORT_WEBSOCKET,
	};

	struct MCPToolDef {
		String name;
		String description;
		Dictionary input_schema;
		Callable handler;
	};

	struct MCPResourceDef {
		String uri;
		String name;
		String description;
		String mime_type;
		Callable handler; // Returns Dictionary with "contents" array.
	};

	struct MCPPromptDef {
		String name;
		String description;
		Array arguments; // Array of Dictionaries: {name, description, required}
		Callable handler; // Returns Dictionary with "messages" array.
	};

private:
	static MCPServer *singleton;

	TransportMode transport_mode = TRANSPORT_NONE;
	bool running = false;
	bool initialized = false;

	// WebSocket transport.
	Ref<TCPServer> tcp_server;
	struct WSClient {
		Ref<WebSocketPeer> ws_peer;
		bool ws_connected = false;
	};
	HashMap<int, WSClient> ws_clients;
	int next_client_id = 1;

	// stdio transport.
	String stdin_buffer;

	// MCP protocol.
	String server_name = "godot-mcp";
	String server_version = "1.0.0";

	// Registries.
	HashMap<String, MCPToolDef> mcp_tools;
	HashMap<String, MCPResourceDef> mcp_resources;
	HashMap<String, MCPPromptDef> mcp_prompts;

	// Event subscriptions: event_type -> set of client_ids.
	HashMap<String, HashSet<int>> event_subscriptions;

	Mutex mutex;

	// Protocol handlers.
	Dictionary _handle_initialize(const Dictionary &p_params, const Variant &p_id);
	Dictionary _handle_initialized_notification();
	Dictionary _handle_ping(const Variant &p_id);

	// Tools protocol.
	Dictionary _handle_tools_list(const Variant &p_id);
	Dictionary _handle_tools_call(const Dictionary &p_params, const Variant &p_id);

	// Resources protocol.
	Dictionary _handle_resources_list(const Variant &p_id);
	Dictionary _handle_resources_read(const Dictionary &p_params, const Variant &p_id);

	// Prompts protocol.
	Dictionary _handle_prompts_list(const Variant &p_id);
	Dictionary _handle_prompts_get(const Dictionary &p_params, const Variant &p_id);

	// Transport helpers.
	void _poll_websocket();
	void _poll_stdio();
	void _process_message(const String &p_message, int p_client_id = -1);
	void _send_response(const Dictionary &p_response, int p_client_id = -1);
	void _send_stdio_message(const String &p_json);
	void _send_ws_message(const String &p_json, int p_client_id);

	// MCP message framing (Content-Length header for stdio).
	String _frame_message(const String &p_json) const;
	bool _try_parse_stdio_message(String &r_message);

	// Registration orchestration.
	void _register_builtin_tools();
	void _register_builtin_resources();
	void _register_builtin_prompts();

protected:
	static void _bind_methods();

public:
	static MCPServer *get_singleton() { return singleton; }

	Error start_websocket(int p_port, const String &p_bind_host = "127.0.0.1");
	Error start_stdio();
	void stop();
	void poll(int p_limit_usec = 0);

	bool is_running() const { return running; }
	TransportMode get_transport_mode() const { return transport_mode; }

	// Tool API.
	void register_tool(const String &p_name, const String &p_description, const Dictionary &p_input_schema, const Callable &p_handler);
	void unregister_tool(const String &p_name);
	TypedArray<Dictionary> get_tool_list() const;

	// Resource API.
	void register_resource(const String &p_uri, const String &p_name, const String &p_description, const String &p_mime_type, const Callable &p_handler);
	void unregister_resource(const String &p_uri);

	// Prompt API.
	void register_prompt(const String &p_name, const String &p_description, const Array &p_arguments, const Callable &p_handler);
	void unregister_prompt(const String &p_name);

	// Event push API.
	void send_notification(const String &p_event_type, const Dictionary &p_data, int p_client_id = -1);
	void broadcast_notification(const String &p_event_type, const Dictionary &p_data);

	MCPServer();
	~MCPServer();
};

VARIANT_ENUM_CAST(MCPServer::TransportMode);
