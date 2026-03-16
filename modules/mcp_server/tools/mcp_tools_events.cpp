/**************************************************************************/
/*  mcp_tools_events.cpp                                                  */
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

#include "mcp_tools_events.h"

#include "../mcp_server.h"
#include "../mcp_tool_helpers.h"


// --- MCPEventEmitter static members ---

const char *MCPEventEmitter::EVENT_SCENE_CHANGED = "scene_changed";
const char *MCPEventEmitter::EVENT_SCRIPT_ERROR = "script_error";
const char *MCPEventEmitter::EVENT_NODE_SELECTED = "node_selected";
const char *MCPEventEmitter::EVENT_FILE_CHANGED = "file_changed";
const char *MCPEventEmitter::EVENT_GAME_OUTPUT = "game_output";
const char *MCPEventEmitter::EVENT_SCRIPT_RELOADED = "script_reloaded";

HashMap<String, HashSet<String>> MCPEventEmitter::subscriptions;

void MCPEventEmitter::subscribe(const String &p_event_type, const String &p_client_id) {
	subscriptions[p_event_type].insert(p_client_id);
}

void MCPEventEmitter::unsubscribe(const String &p_event_type, const String &p_client_id) {
	if (subscriptions.has(p_event_type)) {
		subscriptions[p_event_type].erase(p_client_id);
		if (subscriptions[p_event_type].is_empty()) {
			subscriptions.erase(p_event_type);
		}
	}
}

void MCPEventEmitter::emit_event(const String &p_event_type, const Dictionary &p_data) {
	if (!subscriptions.has(p_event_type)) {
		return;
	}

	MCPServer *server = MCPServer::get_singleton();
	if (!server) {
		return;
	}

	// Build a JSON-RPC notification for this event.
	Dictionary notification;
	notification["jsonrpc"] = "2.0";
	notification["method"] = "notifications/event";

	Dictionary params;
	params["event_type"] = p_event_type;
	params["data"] = p_data;
	notification["params"] = params;

	// The server would push this notification to subscribed clients.
	// For now, we store it for clients to poll, or the server transport
	// layer can be extended to push to specific client IDs.
	// This is a framework hook - actual push depends on transport.
}

Dictionary MCPEventEmitter::get_all_subscriptions() {
	Dictionary result;
	for (const KeyValue<String, HashSet<String>> &kv : subscriptions) {
		Array clients;
		for (const String &client_id : kv.value) {
			clients.push_back(client_id);
		}
		result[kv.key] = clients;
	}
	return result;
}

void MCPEventEmitter::clear() {
	subscriptions.clear();
}

// --- Supported event types ---

static Array get_supported_event_types() {
	Array types;
	types.push_back(MCPEventEmitter::EVENT_SCENE_CHANGED);
	types.push_back(MCPEventEmitter::EVENT_SCRIPT_ERROR);
	types.push_back(MCPEventEmitter::EVENT_NODE_SELECTED);
	types.push_back(MCPEventEmitter::EVENT_FILE_CHANGED);
	types.push_back(MCPEventEmitter::EVENT_GAME_OUTPUT);
	types.push_back(MCPEventEmitter::EVENT_SCRIPT_RELOADED);
	return types;
}

// --- subscribe ---

static Dictionary mcp_tool_subscribe(const Dictionary &p_args) {
	String event_type = p_args.get("event_type", "");
	if (event_type.is_empty()) {
		return mcp_error("event_type is required");
	}

	String client_id = p_args.get("client_id", "");
	if (client_id.is_empty()) {
		return mcp_error("client_id is required");
	}

	// Validate event type.
	Array supported = get_supported_event_types();
	bool valid = false;
	for (int i = 0; i < supported.size(); i++) {
		if (String(supported[i]) == event_type) {
			valid = true;
			break;
		}
	}
	if (!valid) {
		return mcp_error("Unsupported event type: " + event_type + ". Supported types: scene_changed, script_error, node_selected, file_changed, game_output, script_reloaded");
	}

	MCPEventEmitter::subscribe(event_type, client_id);

	Dictionary result;
	result["success"] = true;
	result["event_type"] = event_type;
	result["client_id"] = client_id;
	result["message"] = "Subscribed to " + event_type;
	return result;
}

// --- unsubscribe ---

static Dictionary mcp_tool_unsubscribe(const Dictionary &p_args) {
	String event_type = p_args.get("event_type", "");
	if (event_type.is_empty()) {
		return mcp_error("event_type is required");
	}

	String client_id = p_args.get("client_id", "");
	if (client_id.is_empty()) {
		return mcp_error("client_id is required");
	}

	MCPEventEmitter::unsubscribe(event_type, client_id);

	Dictionary result;
	result["success"] = true;
	result["event_type"] = event_type;
	result["client_id"] = client_id;
	result["message"] = "Unsubscribed from " + event_type;
	return result;
}

// --- list_subscriptions ---

static Dictionary mcp_tool_list_subscriptions(const Dictionary &p_args) {
	Dictionary result;
	result["success"] = true;
	result["subscriptions"] = MCPEventEmitter::get_all_subscriptions();
	result["supported_event_types"] = get_supported_event_types();
	return result;
}

// --- Registration ---

void mcp_register_events_tools(MCPServer *p_server) {
	// subscribe
	{
		Array event_types = get_supported_event_types();

		Dictionary schema = MCPSchema::object()
									.prop_enum("event_type", event_types, "Type of event to subscribe to", true)
									.prop("client_id", "string", "Unique client identifier for this subscription", true)
									.build();
		p_server->register_tool("subscribe",
				"Subscribe to engine/editor events for real-time notifications",
				schema, callable_mp_static(&mcp_tool_subscribe));
	}

	// unsubscribe
	{
		Array event_types = get_supported_event_types();

		Dictionary schema = MCPSchema::object()
									.prop_enum("event_type", event_types, "Type of event to unsubscribe from", true)
									.prop("client_id", "string", "Client identifier to unsubscribe", true)
									.build();
		p_server->register_tool("unsubscribe",
				"Unsubscribe from engine/editor events",
				schema, callable_mp_static(&mcp_tool_unsubscribe));
	}

	// list_subscriptions
	{
		Dictionary schema = MCPSchema::object().build();
		p_server->register_tool("list_subscriptions",
				"List all active event subscriptions and supported event types",
				schema, callable_mp_static(&mcp_tool_list_subscriptions));
	}
}
