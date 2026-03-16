/**************************************************************************/
/*  mcp_tools_events.h                                                    */
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

#include "core/string/ustring.h"
#include "core/templates/hash_map.h"
#include "core/templates/hash_set.h"
#include "core/variant/dictionary.h"
#include "core/variant/array.h"

class MCPServer;

// Event emitter helper that other engine subsystems can use to push
// notifications to MCP clients.
class MCPEventEmitter {
public:
	static const char *EVENT_SCENE_CHANGED;
	static const char *EVENT_SCRIPT_ERROR;
	static const char *EVENT_NODE_SELECTED;
	static const char *EVENT_FILE_CHANGED;
	static const char *EVENT_GAME_OUTPUT;
	static const char *EVENT_SCRIPT_RELOADED;

	// Subscribe a client to an event type.
	static void subscribe(const String &p_event_type, const String &p_client_id);
	// Unsubscribe a client from an event type.
	static void unsubscribe(const String &p_event_type, const String &p_client_id);
	// Emit an event to all subscribed clients.
	static void emit_event(const String &p_event_type, const Dictionary &p_data);
	// Get all current subscriptions.
	static Dictionary get_all_subscriptions();
	// Clear all subscriptions.
	static void clear();

private:
	// Map of event_type -> set of client_ids.
	static HashMap<String, HashSet<String>> subscriptions;
};

void mcp_register_events_tools(MCPServer *p_server);
