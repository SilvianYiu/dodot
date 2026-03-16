/**************************************************************************/
/*  mcp_tools_hot_reload.cpp                                              */
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

#include "mcp_tools_hot_reload.h"

#include "../mcp_server.h"
#include "../mcp_tool_helpers.h"

#include "core/io/resource_loader.h"
#include "core/object/script_language.h"
#include "scene/main/window.h"
#include "scene/resources/packed_scene.h"

#ifdef TOOLS_ENABLED
#include "editor/editor_node.h"
#include "editor/file_system/editor_file_system.h"
#include "editor/scene/editor_scene_tabs.h"
#include "editor/script/script_editor_plugin.h"
#include "editor/settings/editor_settings.h"
#endif

// --- Tool: trigger_rescan ---
// Forces the editor filesystem to rescan for changes immediately.
static Dictionary mcp_tool_trigger_rescan(const Dictionary &p_args) {
#ifdef TOOLS_ENABLED
	EditorFileSystem *efs = EditorFileSystem::get_singleton();
	if (!efs) {
		return mcp_error("EditorFileSystem not available (not in editor mode)");
	}

	efs->scan_changes();

	Dictionary result;
	result["message"] = "Filesystem rescan triggered";
	return mcp_success(result);
#else
	return mcp_error("Only available in editor mode");
#endif
}

// --- Tool: reload_script ---
// Reloads a specific script from disk by path.
static Dictionary mcp_tool_reload_script(const Dictionary &p_args) {
	String path = p_args.get("path", "");
	if (path.is_empty()) {
		return mcp_error("Missing required parameter: path");
	}

	if (!path.ends_with(".gd") && !path.ends_with(".cs") && !path.ends_with(".gdscript")) {
		return mcp_error("Path must be a script file (.gd, .cs)");
	}

	// Check if the resource is loaded in cache.
	if (!ResourceCache::has(path)) {
		return mcp_error("Script not loaded in cache: " + path);
	}

	Ref<Script> script = ResourceLoader::load(path, "Script", ResourceFormatLoader::CACHE_MODE_IGNORE);
	if (script.is_null()) {
		return mcp_error("Failed to load script: " + path);
	}

	// Find the cached version and update it.
	Ref<Resource> cached = ResourceCache::get_ref(path);
	if (cached.is_valid()) {
		Ref<Script> cached_script = cached;
		if (cached_script.is_valid()) {
			cached_script->set_source_code(script->get_source_code());
			Error err = cached_script->reload(true); // true = keep state
			if (err != OK) {
				return mcp_error("Script reload failed with error: " + itos(err));
			}
		}
	}

#ifdef TOOLS_ENABLED
	// Notify the script editor.
	ScriptEditor *se = ScriptEditor::get_singleton();
	if (se) {
		se->reload_scripts();
	}
#endif

	Dictionary result;
	result["path"] = path;
	result["message"] = "Script reloaded successfully";
	return mcp_success(result);
}

// --- Tool: reload_scene ---
// Reloads the current scene or a specific scene from disk.
static Dictionary mcp_tool_reload_scene(const Dictionary &p_args) {
#ifdef TOOLS_ENABLED
	String path = p_args.get("path", "");

	EditorNode *editor = EditorNode::get_singleton();
	if (!editor) {
		return mcp_error("EditorNode not available");
	}

	if (path.is_empty()) {
		// Reload current scene.
		int current_tab = EditorSceneTabs::get_singleton()->get_current_tab();
		if (current_tab < 0) {
			return mcp_error("No scene currently open");
		}
		path = editor->get_edited_scene()->get_scene_file_path();
		if (path.is_empty()) {
			return mcp_error("Current scene has no file path (unsaved)");
		}
	}

	// Force reload from disk.
	editor->reload_scene(path);

	Dictionary result;
	result["path"] = path;
	result["message"] = "Scene reloaded from disk";
	return mcp_success(result);
#else
	return mcp_error("Only available in editor mode");
#endif
}

#ifdef TOOLS_ENABLED
static void _find_modified_recursive(EditorFileSystemDirectory *p_dir, const Vector<String> &p_extensions, Array &r_modified);
#endif

// --- Tool: get_modified_files ---
// Lists files that have been modified on disk since last scan.
static Dictionary mcp_tool_get_modified_files(const Dictionary &p_args) {
#ifdef TOOLS_ENABLED
	EditorFileSystem *efs = EditorFileSystem::get_singleton();
	if (!efs) {
		return mcp_error("EditorFileSystem not available");
	}

	String filter = p_args.get("filter", "");

	// Scan for changes first.
	efs->scan_changes();

	// Get list of all files and check modification times.
	Array modified;
	Vector<String> extensions;
	if (filter == "scripts") {
		extensions.push_back("gd");
		extensions.push_back("cs");
	} else if (filter == "scenes") {
		extensions.push_back("tscn");
		extensions.push_back("scn");
	} else if (filter == "resources") {
		extensions.push_back("tres");
		extensions.push_back("res");
	}

	// Walk the filesystem and check for loaded resources with stale timestamps.
	_find_modified_recursive(efs->get_filesystem(), extensions, modified);

	Dictionary result;
	result["modified_files"] = modified;
	result["count"] = modified.size();
	return mcp_success(result);
#else
	return mcp_error("Only available in editor mode");
#endif
}

#ifdef TOOLS_ENABLED
static void _find_modified_recursive(EditorFileSystemDirectory *p_dir, const Vector<String> &p_extensions, Array &r_modified) {
	if (!p_dir) {
		return;
	}

	for (int i = 0; i < p_dir->get_file_count(); i++) {
		String path = p_dir->get_file_path(i);
		String ext = path.get_extension();

		if (!p_extensions.is_empty()) {
			bool match = false;
			for (const String &e : p_extensions) {
				if (ext == e) {
					match = true;
					break;
				}
			}
			if (!match) {
				continue;
			}
		}

		// Check if this resource is loaded and might be stale.
		if (ResourceCache::has(path)) {
			uint64_t disk_time = FileAccess::get_modified_time(path);
			Dictionary entry;
			entry["path"] = path;
			entry["type"] = p_dir->get_file_type(i);
			entry["modified_time"] = disk_time;
			entry["in_cache"] = true;
			r_modified.push_back(entry);
		}
	}

	for (int i = 0; i < p_dir->get_subdir_count(); i++) {
		_find_modified_recursive(p_dir->get_subdir(i), p_extensions, r_modified);
	}
}
#endif

// --- Tool: reload_all_scripts ---
// Reloads all currently loaded scripts from disk.
static Dictionary mcp_tool_reload_all_scripts(const Dictionary &p_args) {
#ifdef TOOLS_ENABLED
	ScriptEditor *se = ScriptEditor::get_singleton();
	if (!se) {
		return mcp_error("ScriptEditor not available");
	}

	se->reload_scripts();

	Dictionary result;
	result["message"] = "All open scripts reloaded";
	return mcp_success(result);
#else
	return mcp_error("Only available in editor mode");
#endif
}

// --- Tool: set_auto_reload ---
// Configure auto-reload behavior for AI workflow.
static Dictionary mcp_tool_set_auto_reload(const Dictionary &p_args) {
#ifdef TOOLS_ENABLED
	bool scripts = p_args.get("scripts", true);
	bool scenes = p_args.get("scenes", false);

	EditorSettings *settings = EditorSettings::get_singleton();
	if (!settings) {
		return mcp_error("EditorSettings not available");
	}

	settings->set("text_editor/behavior/files/auto_reload_scripts_on_external_change", scripts);
	settings->set("text_editor/behavior/files/auto_reload_and_parse_scripts_on_save", scripts);

	Dictionary result;
	result["auto_reload_scripts"] = scripts;
	result["auto_reload_scenes"] = scenes;
	result["message"] = "Auto-reload settings updated";
	return mcp_success(result);
#else
	return mcp_error("Only available in editor mode");
#endif
}

// --- Registration ---

void mcp_register_hot_reload_tools(MCPServer *p_server) {
	p_server->register_tool("trigger_rescan",
			"Force the editor filesystem to rescan for file changes immediately",
			MCPSchema::object().build(),
			callable_mp_static(&mcp_tool_trigger_rescan));

	p_server->register_tool("reload_script",
			"Reload a specific script from disk (with state preservation)",
			MCPSchema::object()
					.prop("path", "string", "Resource path of the script (e.g. res://player.gd)", true)
					.build(),
			callable_mp_static(&mcp_tool_reload_script));

	p_server->register_tool("reload_scene",
			"Reload the current scene or a specific scene from disk",
			MCPSchema::object()
					.prop("path", "string", "Resource path of the scene (empty = current scene)")
					.build(),
			callable_mp_static(&mcp_tool_reload_scene));

	p_server->register_tool("get_modified_files",
			"List files that are loaded in cache (optionally filtered by type)",
			MCPSchema::object()
					.prop("filter", "string", "Filter type: 'scripts', 'scenes', 'resources', or empty for all")
					.build(),
			callable_mp_static(&mcp_tool_get_modified_files));

	p_server->register_tool("reload_all_scripts",
			"Reload all currently open scripts from disk",
			MCPSchema::object().build(),
			callable_mp_static(&mcp_tool_reload_all_scripts));

	p_server->register_tool("set_auto_reload",
			"Configure auto-reload behavior for AI workflow",
			MCPSchema::object()
					.prop("scripts", "boolean", "Enable auto-reload for scripts (default: true)")
					.prop("scenes", "boolean", "Enable auto-reload for scenes (default: false)")
					.build(),
			callable_mp_static(&mcp_tool_set_auto_reload));
}
