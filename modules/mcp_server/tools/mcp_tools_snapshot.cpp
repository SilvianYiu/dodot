/**************************************************************************/
/*  mcp_tools_snapshot.cpp                                                */
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

#include "mcp_tools_snapshot.h"

#include "../mcp_server.h"
#include "../mcp_tool_helpers.h"

#include "core/os/os.h"
#include "scene/resources/packed_scene.h"

// Snapshot storage.
struct SnapshotEntry {
	String label;
	Ref<PackedScene> packed_scene;
	uint64_t timestamp = 0;
};

static HashMap<int, SnapshotEntry> snapshot_store;
static int next_snapshot_id = 1;

// --- snapshot_scene ---

static Dictionary mcp_tool_snapshot_scene(const Dictionary &p_args) {
	String label = p_args.get("label", "");

	SceneTree *st = mcp_get_scene_tree();
	if (!st) {
		return mcp_error("No SceneTree available");
	}

	Node *root = st->get_root();
	if (!root) {
		return mcp_error("No root node available");
	}

	// Find the current scene root (first child of root that is the main scene).
	Node *scene_root = st->get_current_scene();
	if (!scene_root) {
		// Fallback: use root directly.
		scene_root = root;
	}

	Ref<PackedScene> packed;
	packed.instantiate();

	Error err = packed->pack(scene_root);
	if (err != OK) {
		return mcp_error("Failed to pack scene, error code: " + itos(err));
	}

	int id = next_snapshot_id++;

	SnapshotEntry entry;
	entry.label = label.is_empty() ? ("snapshot_" + itos(id)) : label;
	entry.packed_scene = packed;
	entry.timestamp = OS::get_singleton()->get_ticks_msec();

	snapshot_store[id] = entry;

	Dictionary result;
	result["success"] = true;
	result["snapshot_id"] = id;
	result["label"] = entry.label;
	return result;
}

// --- list_snapshots ---

static Dictionary mcp_tool_list_snapshots(const Dictionary &p_args) {
	Dictionary result;
	result["success"] = true;

	Array snapshots;
	for (const KeyValue<int, SnapshotEntry> &kv : snapshot_store) {
		Dictionary snap;
		snap["id"] = kv.key;
		snap["label"] = kv.value.label;
		snap["timestamp_ms"] = kv.value.timestamp;
		snapshots.push_back(snap);
	}
	result["snapshots"] = snapshots;
	result["count"] = snapshots.size();
	return result;
}

// --- restore_snapshot ---

static Dictionary mcp_tool_restore_snapshot(const Dictionary &p_args) {
	int snapshot_id = p_args.get("snapshot_id", -1);
	if (snapshot_id < 0) {
		return mcp_error("snapshot_id is required");
	}

	if (!snapshot_store.has(snapshot_id)) {
		return mcp_error("Snapshot not found with id: " + itos(snapshot_id));
	}

	const SnapshotEntry &entry = snapshot_store[snapshot_id];

	SceneTree *st = mcp_get_scene_tree();
	if (!st) {
		return mcp_error("No SceneTree available");
	}

	Node *current_scene = st->get_current_scene();
	if (!current_scene) {
		return mcp_error("No current scene to replace");
	}

	// Instantiate the snapshot.
	Node *restored = entry.packed_scene->instantiate();
	if (!restored) {
		return mcp_error("Failed to instantiate snapshot");
	}

	// Replace the current scene.
	Node *root = st->get_root();

	// Remove the current scene.
	root->remove_child(current_scene);
	memdelete(current_scene);

	// Add the restored scene.
	root->add_child(restored);
	st->set_current_scene(restored);

	Dictionary result;
	result["success"] = true;
	result["snapshot_id"] = snapshot_id;
	result["label"] = entry.label;
	result["message"] = "Scene restored from snapshot";
	return result;
}

// --- delete_snapshot ---

static Dictionary mcp_tool_delete_snapshot(const Dictionary &p_args) {
	int snapshot_id = p_args.get("snapshot_id", -1);
	if (snapshot_id < 0) {
		return mcp_error("snapshot_id is required");
	}

	if (!snapshot_store.has(snapshot_id)) {
		return mcp_error("Snapshot not found with id: " + itos(snapshot_id));
	}

	String label = snapshot_store[snapshot_id].label;
	snapshot_store.erase(snapshot_id);

	Dictionary result;
	result["success"] = true;
	result["snapshot_id"] = snapshot_id;
	result["label"] = label;
	result["message"] = "Snapshot deleted";
	return result;
}

// --- Registration ---

void mcp_register_snapshot_tools(MCPServer *p_server) {
	// snapshot_scene
	{
		Dictionary schema = MCPSchema::object()
									.prop("label", "string", "Optional label for the snapshot")
									.build();
		p_server->register_tool("snapshot_scene",
				"Save a snapshot of the current scene state for later rollback",
				schema, callable_mp_static(&mcp_tool_snapshot_scene));
	}

	// list_snapshots
	{
		Dictionary schema = MCPSchema::object().build();
		p_server->register_tool("list_snapshots",
				"List all saved scene snapshots",
				schema, callable_mp_static(&mcp_tool_list_snapshots));
	}

	// restore_snapshot
	{
		Dictionary schema = MCPSchema::object()
									.prop_number("snapshot_id", "ID of the snapshot to restore", true)
									.build();
		p_server->register_tool("restore_snapshot",
				"Restore the scene to a previously saved snapshot",
				schema, callable_mp_static(&mcp_tool_restore_snapshot));
	}

	// delete_snapshot
	{
		Dictionary schema = MCPSchema::object()
									.prop_number("snapshot_id", "ID of the snapshot to delete", true)
									.build();
		p_server->register_tool("delete_snapshot",
				"Delete a saved scene snapshot",
				schema, callable_mp_static(&mcp_tool_delete_snapshot));
	}
}
