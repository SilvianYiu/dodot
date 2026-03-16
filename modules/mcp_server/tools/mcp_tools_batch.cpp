/**************************************************************************/
/*  mcp_tools_batch.cpp                                                   */
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

#include "mcp_tools_batch.h"

#include "../mcp_server.h"
#include "../mcp_tool_helpers.h"

#include "scene/resources/packed_scene.h"

// --- batch_execute ---

static Dictionary mcp_tool_batch_execute(const Dictionary &p_args) {
	Array operations = p_args.get("operations", Array());
	bool atomic = p_args.get("atomic", false);

	if (operations.is_empty()) {
		return mcp_error("operations array is required and must not be empty");
	}

	MCPServer *server = MCPServer::get_singleton();
	if (!server) {
		return mcp_error("MCPServer singleton not available");
	}

	// If atomic, take a snapshot of the current scene before starting.
	Ref<PackedScene> pre_snapshot;
	Node *scene_root = nullptr;

	if (atomic) {
		SceneTree *st = mcp_get_scene_tree();
		if (st) {
			scene_root = st->get_current_scene();
			if (scene_root) {
				pre_snapshot.instantiate();
				Error err = pre_snapshot->pack(scene_root);
				if (err != OK) {
					pre_snapshot.unref();
				}
			}
		}
	}

	// Get the tool list to look up handlers.
	TypedArray<Dictionary> tool_list = server->get_tool_list();
	HashMap<String, int> tool_index;
	for (int i = 0; i < tool_list.size(); i++) {
		Dictionary td = tool_list[i];
		tool_index[td.get("name", "")] = i;
	}

	Array results;
	bool had_failure = false;

	for (int i = 0; i < operations.size(); i++) {
		Dictionary op = operations[i];

		String tool_name = op.get("tool", "");
		Dictionary arguments = op.get("arguments", Dictionary());

		if (tool_name.is_empty()) {
			Dictionary op_result;
			op_result["index"] = i;
			op_result["error"] = "Operation missing 'tool' field";
			results.push_back(op_result);
			had_failure = true;

			if (atomic) {
				break;
			}
			continue;
		}

		// Use the server's internal tool call mechanism.
		// Build a params dict as _handle_tools_call expects.
		Dictionary call_params;
		call_params["name"] = tool_name;
		call_params["arguments"] = arguments;

		// Call the tool through the server's handler.
		Dictionary tool_result;

		// Look up the tool in the registry and call its handler directly.
		// We access the handler via the registered tools.
		// The MCPServer stores tools in mcp_tools HashMap, but we can use
		// _handle_tools_call indirectly. Instead, let's call register_tool's
		// handler callable directly.

		// We need to find the tool and call it. The get_tool_list returns
		// metadata but not the handler. We'll use the fact that
		// _handle_tools_call processes tool calls.
		// For simplicity and encapsulation, we simulate a tools/call request.
		Dictionary response = server->call("_handle_tools_call", call_params, i);
		if (response.has("error")) {
			// Fallback: try to extract content from the response structure.
			tool_result = response;
			had_failure = true;
		} else if (response.has("result")) {
			Dictionary resp_result = response["result"];
			if (resp_result.has("content")) {
				Array content = resp_result["content"];
				if (content.size() > 0) {
					Dictionary first_content = content[0];
					tool_result["result"] = first_content.get("text", "");
				}
			}
		} else {
			tool_result = response;
		}

		Dictionary op_result;
		op_result["index"] = i;
		op_result["tool"] = tool_name;
		op_result["result"] = tool_result;

		if (tool_result.has("error")) {
			had_failure = true;
			op_result["status"] = "error";
		} else {
			op_result["status"] = "success";
		}

		results.push_back(op_result);

		if (atomic && had_failure) {
			break;
		}
	}

	// If atomic and there was a failure, rollback.
	if (atomic && had_failure && pre_snapshot.is_valid() && scene_root) {
		SceneTree *st = mcp_get_scene_tree();
		if (st) {
			Node *current = st->get_current_scene();
			Node *restored = pre_snapshot->instantiate();

			if (restored && current) {
				Node *root = st->get_root();
				root->remove_child(current);
				memdelete(current);
				root->add_child(restored);
				st->set_current_scene(restored);
			}
		}

		Dictionary result;
		result["success"] = false;
		result["atomic"] = true;
		result["rolled_back"] = true;
		result["message"] = "Batch operation failed, all changes have been rolled back";
		result["results"] = results;
		return result;
	}

	Dictionary result;
	result["success"] = !had_failure;
	result["atomic"] = atomic;
	result["results"] = results;
	result["total_operations"] = operations.size();
	result["completed_operations"] = results.size();
	return result;
}

// --- Registration ---

void mcp_register_batch_tools(MCPServer *p_server) {
	// batch_execute
	{
		Dictionary schema = MCPSchema::object()
									.prop_array("operations", "Array of operation dicts, each with 'tool' (string) and 'arguments' (object)", true)
									.prop_bool("atomic", "If true, rollback all changes on any failure (default: false)")
									.build();
		p_server->register_tool("batch_execute",
				"Execute multiple tool calls in sequence. If atomic is true, all changes are rolled back on failure.",
				schema, callable_mp_static(&mcp_tool_batch_execute));
	}
}
