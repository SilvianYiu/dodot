/**************************************************************************/
/*  mcp_tools_core.cpp                                                    */
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

#include "mcp_tools_core.h"

#include "../mcp_server.h"
#include "../mcp_tool_helpers.h"

#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "core/version.h"
#include "scene/resources/packed_scene.h"

#ifdef TOOLS_ENABLED
#include "editor/editor_node.h"
#include "editor/run/editor_run_bar.h"
#endif

// --- Tool Handlers ---

static Dictionary mcp_tool_ping(const Dictionary &p_args) {
	Dictionary result;
	result["status"] = "ok";
	result["message"] = "pong";
	return result;
}

static Dictionary mcp_tool_get_engine_info(const Dictionary &p_args) {
	Dictionary result;
	result["engine"] = "Godot Engine";
	result["version_major"] = VERSION_MAJOR;
	result["version_minor"] = VERSION_MINOR;
	result["version_patch"] = VERSION_PATCH;
	result["version_status"] = VERSION_STATUS;
	result["version_string"] = String(VERSION_FULL_NAME);
	result["mcp_server_version"] = "1.0.0";
	return result;
}

static Dictionary mcp_tool_get_project_structure(const Dictionary &p_args) {
	Dictionary result;
	Array files;

	String project_path = ProjectSettings::get_singleton()->get_resource_path();
	if (project_path.is_empty()) {
		return mcp_error("No project loaded");
	}

	// Recursively list files under res://
	Vector<String> dirs_to_scan;
	dirs_to_scan.push_back(project_path);

	while (!dirs_to_scan.is_empty()) {
		String dir_path = dirs_to_scan[dirs_to_scan.size() - 1];
		dirs_to_scan.remove_at(dirs_to_scan.size() - 1);

		Ref<DirAccess> da = DirAccess::open(dir_path);
		if (da.is_null()) {
			continue;
		}

		da->list_dir_begin();
		String fname = da->get_next();
		while (!fname.is_empty()) {
			if (fname.begins_with(".")) {
				fname = da->get_next();
				continue;
			}
			String full_path = dir_path.path_join(fname);
			if (da->current_is_dir()) {
				dirs_to_scan.push_back(full_path);
			} else {
				// Convert to res:// path.
				String res_path = full_path.replace(project_path, "res:/");
				if (!res_path.begins_with("res://")) {
					res_path = "res://" + res_path.substr(6);
				}
				Dictionary file_info;
				file_info["path"] = res_path;
				file_info["extension"] = fname.get_extension();
				files.push_back(file_info);
			}
			fname = da->get_next();
		}
		da->list_dir_end();
	}

	result["project_path"] = project_path;
	result["files"] = files;
	result["file_count"] = files.size();
	return result;
}

static Dictionary mcp_tool_get_scene_tree(const Dictionary &p_args) {
	SceneTree *scene_tree = mcp_get_scene_tree();
	if (!scene_tree) {
		return mcp_error("No SceneTree available");
	}

	Node *root = scene_tree->get_root();
	if (!root) {
		return mcp_error("No root node");
	}

	struct NodeInfo {
		static Dictionary get_node_dict(Node *p_node) {
			Dictionary d;
			d["name"] = p_node->get_name();
			d["type"] = p_node->get_class();
			d["path"] = String(p_node->get_path());

			Array children;
			for (int i = 0; i < p_node->get_child_count(); i++) {
				children.push_back(get_node_dict(p_node->get_child(i)));
			}
			if (!children.is_empty()) {
				d["children"] = children;
			}

			Ref<Script> script = p_node->get_script();
			if (script.is_valid()) {
				d["script"] = script->get_path();
			}

			return d;
		}
	};

	Dictionary result;
	result["scene_tree"] = NodeInfo::get_node_dict(root);
	return result;
}

static Dictionary mcp_tool_create_node(const Dictionary &p_args) {
	String parent_path = p_args.get("parent_path", "/root");
	String type = p_args.get("type", "Node");
	String name = p_args.get("name", "");

	Dictionary err;
	Node *parent = mcp_get_node(parent_path, err);
	if (!parent) {
		return err;
	}

	Node *new_node = mcp_create_node_of_type(type, name, err);
	if (!new_node) {
		return err;
	}

	// Set properties if provided.
	Dictionary properties = p_args.get("properties", Dictionary());
	mcp_set_properties(new_node, properties);

	mcp_add_child_to_scene(parent, new_node);

	Dictionary result;
	result["success"] = true;
	result["path"] = String(new_node->get_path());
	result["name"] = new_node->get_name();
	result["type"] = type;
	return result;
}

static Dictionary mcp_tool_delete_node(const Dictionary &p_args) {
	String node_path = p_args.get("node_path", "");
	if (node_path.is_empty()) {
		return mcp_error("node_path is required");
	}

	SceneTree *scene_tree = mcp_get_scene_tree();
	if (!scene_tree) {
		return mcp_error("No SceneTree available");
	}

	Dictionary err;
	Node *node = mcp_get_node(node_path, err);
	if (!node) {
		return err;
	}

	if (node == scene_tree->get_root()) {
		return mcp_error("Cannot delete root node");
	}

	node->queue_free();

	Dictionary result;
	result["success"] = true;
	result["deleted"] = node_path;
	return result;
}

static Dictionary mcp_tool_modify_node(const Dictionary &p_args) {
	String node_path = p_args.get("node_path", "");
	if (node_path.is_empty()) {
		return mcp_error("node_path is required");
	}

	Dictionary properties = p_args.get("properties", Dictionary());

	Dictionary err;
	Node *node = mcp_get_node(node_path, err);
	if (!node) {
		return err;
	}

	Array modified;
	for (const Variant *key = properties.next(nullptr); key; key = properties.next(key)) {
		node->set(StringName(*key), properties[*key]);
		modified.push_back(*key);
	}

	Dictionary result;
	result["success"] = true;
	result["path"] = String(node->get_path());
	result["modified_properties"] = modified;
	return result;
}

static Dictionary mcp_tool_save_scene(const Dictionary &p_args) {
	String path = p_args.get("path", "");
	if (path.is_empty()) {
		return mcp_error("path is required");
	}

	SceneTree *scene_tree = mcp_get_scene_tree();
	if (!scene_tree) {
		return mcp_error("No SceneTree available");
	}

	Node *scene_root = scene_tree->get_edited_scene_root();
	if (!scene_root) {
		scene_root = scene_tree->get_root();
	}

	Ref<PackedScene> packed_scene;
	packed_scene.instantiate();
	Error pack_err = packed_scene->pack(scene_root);
	if (pack_err != OK) {
		return mcp_error("Failed to pack scene");
	}

	Error save_err = ResourceSaver::save(packed_scene, path);
	if (save_err != OK) {
		return mcp_error("Failed to save scene to: " + path);
	}

	Dictionary result;
	result["success"] = true;
	result["path"] = path;
	return result;
}

static Dictionary mcp_tool_load_scene(const Dictionary &p_args) {
	String path = p_args.get("path", "");
	if (path.is_empty()) {
		return mcp_error("path is required");
	}

	SceneTree *scene_tree = mcp_get_scene_tree();
	if (!scene_tree) {
		return mcp_error("No SceneTree available");
	}

	Error err = scene_tree->change_scene_to_file(path);
	if (err != OK) {
		return mcp_error("Failed to load scene: " + path);
	}

	Dictionary result;
	result["success"] = true;
	result["path"] = path;
	return result;
}

static Dictionary mcp_tool_create_script(const Dictionary &p_args) {
	String path = p_args.get("path", "");
	String content = p_args.get("content", "");

	if (path.is_empty()) {
		return mcp_error("path is required");
	}

	Ref<FileAccess> fa = FileAccess::open(path, FileAccess::WRITE);
	if (fa.is_null()) {
		return mcp_error("Failed to open file for writing: " + path);
	}

	fa->store_string(content);
	fa.unref();

	Dictionary result;
	result["success"] = true;
	result["path"] = path;
	return result;
}

static Dictionary mcp_tool_edit_script(const Dictionary &p_args) {
	String path = p_args.get("path", "");
	String content = p_args.get("content", "");

	if (path.is_empty()) {
		return mcp_error("path is required");
	}

	if (!FileAccess::exists(path)) {
		return mcp_error("File does not exist: " + path);
	}

	Ref<FileAccess> fa = FileAccess::open(path, FileAccess::WRITE);
	if (fa.is_null()) {
		return mcp_error("Failed to open file for writing: " + path);
	}

	fa->store_string(content);
	fa.unref();

	Dictionary result;
	result["success"] = true;
	result["path"] = path;
	return result;
}

static Dictionary mcp_tool_validate_script(const Dictionary &p_args) {
	String path = p_args.get("path", "");
	if (path.is_empty()) {
		return mcp_error("path is required");
	}

	if (!FileAccess::exists(path)) {
		return mcp_error("File does not exist: " + path);
	}

	Dictionary result;
	Ref<Script> script = ResourceLoader::load(path);
	result["path"] = path;
	if (script.is_valid()) {
		Error err = script->reload(true);
		if (err == OK) {
			result["valid"] = true;
			result["errors"] = Array();
		} else {
			result["valid"] = false;
			Array errors;
			Dictionary error_info;
			error_info["message"] = "Script validation failed";
			errors.push_back(error_info);
			result["errors"] = errors;
		}
	} else {
		result["valid"] = false;
		Array errors;
		Dictionary error_info;
		error_info["message"] = "Failed to load script";
		errors.push_back(error_info);
		result["errors"] = errors;
	}

	return result;
}

static Dictionary mcp_tool_get_script_symbols(const Dictionary &p_args) {
	String path = p_args.get("path", "");
	if (path.is_empty()) {
		return mcp_error("path is required");
	}

	if (!FileAccess::exists(path)) {
		return mcp_error("File does not exist: " + path);
	}

	String source = FileAccess::get_file_as_string(path);

	Array functions;
	Array variables;
	Array signals;
	Array classes;

	Vector<String> lines = source.split("\n");
	for (int i = 0; i < lines.size(); i++) {
		String line = lines[i].strip_edges();
		if (line.begins_with("func ")) {
			Dictionary sym;
			sym["name"] = line.substr(5, line.find("(") - 5).strip_edges();
			sym["line"] = i + 1;
			sym["signature"] = line;
			functions.push_back(sym);
		} else if (line.begins_with("var ") || line.begins_with("@export var ")) {
			Dictionary sym;
			String var_part = line.begins_with("@export") ? line.substr(line.find("var ") + 4) : line.substr(4);
			int colon = var_part.find(":");
			int eq = var_part.find("=");
			int end = (colon >= 0 && (eq < 0 || colon < eq)) ? colon : (eq >= 0 ? eq : var_part.length());
			sym["name"] = var_part.substr(0, end).strip_edges();
			sym["line"] = i + 1;
			sym["exported"] = line.begins_with("@export");
			variables.push_back(sym);
		} else if (line.begins_with("signal ")) {
			Dictionary sym;
			int paren = line.find("(");
			sym["name"] = line.substr(7, (paren >= 0 ? paren : line.length()) - 7).strip_edges();
			sym["line"] = i + 1;
			signals.push_back(sym);
		} else if (line.begins_with("class ")) {
			Dictionary sym;
			int colon = line.find(":");
			sym["name"] = line.substr(6, (colon >= 0 ? colon : line.length()) - 6).strip_edges();
			sym["line"] = i + 1;
			classes.push_back(sym);
		}
	}

	Dictionary result;
	result["path"] = path;
	result["functions"] = functions;
	result["variables"] = variables;
	result["signals"] = signals;
	result["classes"] = classes;
	return result;
}

static Dictionary mcp_tool_attach_script(const Dictionary &p_args) {
	String node_path = p_args.get("node_path", "");
	String script_path = p_args.get("script_path", "");

	if (node_path.is_empty() || script_path.is_empty()) {
		return mcp_error("node_path and script_path are required");
	}

	Dictionary err;
	Node *node = mcp_get_node(node_path, err);
	if (!node) {
		return err;
	}

	Ref<Script> script = ResourceLoader::load(script_path);
	if (script.is_null()) {
		return mcp_error("Failed to load script: " + script_path);
	}

	node->set_script(script);

	Dictionary result;
	result["success"] = true;
	result["node_path"] = node_path;
	result["script_path"] = script_path;
	return result;
}

static Dictionary mcp_tool_get_project_settings(const Dictionary &p_args) {
	ProjectSettings *ps = ProjectSettings::get_singleton();
	if (!ps) {
		return mcp_error("ProjectSettings not available");
	}

	Dictionary settings;
	settings["application/config/name"] = ps->get_setting("application/config/name", "");
	settings["application/run/main_scene"] = ps->get_setting("application/run/main_scene", "");
	settings["display/window/size/viewport_width"] = ps->get_setting("display/window/size/viewport_width", 1152);
	settings["display/window/size/viewport_height"] = ps->get_setting("display/window/size/viewport_height", 648);
	settings["rendering/renderer/rendering_method"] = ps->get_setting("rendering/renderer/rendering_method", "forward_plus");

	Dictionary result;
	result["settings"] = settings;
	return result;
}

static Dictionary mcp_tool_set_project_setting(const Dictionary &p_args) {
	String key = p_args.get("key", "");
	Variant value = p_args.get("value", Variant());

	if (key.is_empty()) {
		return mcp_error("key is required");
	}

	ProjectSettings *ps = ProjectSettings::get_singleton();
	if (!ps) {
		return mcp_error("ProjectSettings not available");
	}

	ps->set_setting(key, value);
	Error err = ps->save();

	Dictionary result;
	result["success"] = err == OK;
	result["key"] = key;
	return result;
}

static Dictionary mcp_tool_run_project(const Dictionary &p_args) {
	Dictionary result;

#ifdef TOOLS_ENABLED
	EditorRunBar *run_bar = EditorRunBar::get_singleton();
	if (run_bar) {
		run_bar->call_deferred("play_main_scene");
		result["success"] = true;
		result["message"] = "Project run started";
	} else {
		return mcp_error("EditorRunBar not available");
	}
#else
	return mcp_error("Only available in editor builds");
#endif

	return result;
}

static Dictionary mcp_tool_stop_project(const Dictionary &p_args) {
	Dictionary result;

#ifdef TOOLS_ENABLED
	EditorRunBar *run_bar = EditorRunBar::get_singleton();
	if (run_bar) {
		run_bar->call_deferred("stop_playing");
		result["success"] = true;
		result["message"] = "Project stopped";
	} else {
		return mcp_error("EditorRunBar not available");
	}
#else
	return mcp_error("Only available in editor builds");
#endif

	return result;
}

static Dictionary mcp_tool_run_scene(const Dictionary &p_args) {
	String path = p_args.get("path", "");
	if (path.is_empty()) {
		return mcp_error("path is required");
	}

	Dictionary result;

#ifdef TOOLS_ENABLED
	EditorRunBar *run_bar = EditorRunBar::get_singleton();
	if (run_bar) {
		run_bar->call_deferred("play_custom_scene", path);
		result["success"] = true;
		result["path"] = path;
	} else {
		return mcp_error("EditorRunBar not available");
	}
#else
	return mcp_error("Only available in editor builds");
#endif

	return result;
}

static Dictionary mcp_tool_get_debug_output(const Dictionary &p_args) {
	Dictionary result;
	result["message"] = "Debug output is streamed via structured logger. Use --structured-output flag.";
	return result;
}

static Dictionary mcp_tool_get_errors(const Dictionary &p_args) {
	Dictionary result;
	result["message"] = "Errors are streamed via structured logger. Use --structured-output flag.";
	return result;
}

static Dictionary mcp_tool_get_editor_state(const Dictionary &p_args) {
	Dictionary result;

#ifdef TOOLS_ENABLED
	EditorNode *editor = EditorNode::get_singleton();
	if (!editor) {
		return mcp_error("EditorNode not available");
	}

	result["editor_available"] = true;

	Node *edited_scene = editor->get_edited_scene();
	if (edited_scene) {
		result["edited_scene"] = edited_scene->get_scene_file_path();
		result["edited_scene_root"] = edited_scene->get_name();
	}
#else
	return mcp_error("Only available in editor builds");
#endif

	return result;
}

static Dictionary mcp_tool_take_screenshot(const Dictionary &p_args) {
	return mcp_error("Screenshot capture not yet implemented");
}

// --- Registration ---

void mcp_register_core_tools(MCPServer *p_server) {
	// Utility tools.
	p_server->register_tool("ping", "Health check - returns pong",
			MCPSchema::object().build(),
			callable_mp_static(&mcp_tool_ping));

	p_server->register_tool("get_engine_info", "Get Godot engine version and build information",
			MCPSchema::object().build(),
			callable_mp_static(&mcp_tool_get_engine_info));

	// Project tools.
	p_server->register_tool("get_project_structure", "Get the complete file tree under res://",
			MCPSchema::object().build(),
			callable_mp_static(&mcp_tool_get_project_structure));

	p_server->register_tool("get_project_settings", "Get key project settings",
			MCPSchema::object().build(),
			callable_mp_static(&mcp_tool_get_project_settings));

	p_server->register_tool("set_project_setting", "Modify a project setting",
			MCPSchema::object()
					.prop("key", "string", "Setting key path (e.g. application/config/name)", true)
					.prop("value", "string", "Setting value", true)
					.build(),
			callable_mp_static(&mcp_tool_set_project_setting));

	// Scene tools.
	p_server->register_tool("get_scene_tree", "Get the complete scene tree as JSON",
			MCPSchema::object().build(),
			callable_mp_static(&mcp_tool_get_scene_tree));

	p_server->register_tool("create_node", "Create a new node in the scene tree",
			MCPSchema::object()
					.prop("parent_path", "string", "Path to parent node", true)
					.prop("type", "string", "Node type to create (e.g. Node2D, Sprite2D)", true)
					.prop("name", "string", "Name for the new node")
					.prop_object("properties", "Properties to set on the new node")
					.build(),
			callable_mp_static(&mcp_tool_create_node));

	p_server->register_tool("delete_node", "Delete a node from the scene tree",
			MCPSchema::object()
					.prop("node_path", "string", "Path to the node to delete", true)
					.build(),
			callable_mp_static(&mcp_tool_delete_node));

	p_server->register_tool("modify_node", "Modify properties of a node",
			MCPSchema::object()
					.prop("node_path", "string", "Path to the node to modify", true)
					.prop_object("properties", "Properties to modify", true)
					.build(),
			callable_mp_static(&mcp_tool_modify_node));

	p_server->register_tool("save_scene", "Save the current scene to a file",
			MCPSchema::object()
					.prop("path", "string", "File path to save the scene (e.g. res://main.tscn)", true)
					.build(),
			callable_mp_static(&mcp_tool_save_scene));

	p_server->register_tool("load_scene", "Load a scene from a file",
			MCPSchema::object()
					.prop("path", "string", "Path to the scene file to load", true)
					.build(),
			callable_mp_static(&mcp_tool_load_scene));

	// Script tools.
	p_server->register_tool("create_script", "Create a new GDScript file",
			MCPSchema::object()
					.prop("path", "string", "File path for the new script", true)
					.prop("content", "string", "Script content", true)
					.build(),
			callable_mp_static(&mcp_tool_create_script));

	p_server->register_tool("edit_script", "Replace the content of an existing script",
			MCPSchema::object()
					.prop("path", "string", "Path to the script file", true)
					.prop("content", "string", "New script content", true)
					.build(),
			callable_mp_static(&mcp_tool_edit_script));

	p_server->register_tool("validate_script", "Validate a GDScript file for syntax and semantic errors",
			MCPSchema::object()
					.prop("path", "string", "Path to the script file to validate", true)
					.build(),
			callable_mp_static(&mcp_tool_validate_script));

	p_server->register_tool("get_script_symbols", "Get functions, variables, signals, and classes from a script",
			MCPSchema::object()
					.prop("path", "string", "Path to the script file", true)
					.build(),
			callable_mp_static(&mcp_tool_get_script_symbols));

	p_server->register_tool("attach_script", "Attach a script to a node",
			MCPSchema::object()
					.prop("node_path", "string", "Path to the node", true)
					.prop("script_path", "string", "Path to the script file", true)
					.build(),
			callable_mp_static(&mcp_tool_attach_script));

	// Execution tools.
	p_server->register_tool("run_project", "Start running the project",
			MCPSchema::object().build(),
			callable_mp_static(&mcp_tool_run_project));

	p_server->register_tool("stop_project", "Stop the running project",
			MCPSchema::object().build(),
			callable_mp_static(&mcp_tool_stop_project));

	p_server->register_tool("run_scene", "Run a specific scene",
			MCPSchema::object()
					.prop("path", "string", "Path to the scene to run", true)
					.build(),
			callable_mp_static(&mcp_tool_run_scene));

	p_server->register_tool("get_debug_output", "Get recent debug output",
			MCPSchema::object().build(),
			callable_mp_static(&mcp_tool_get_debug_output));

	p_server->register_tool("get_errors", "Get recent script errors",
			MCPSchema::object().build(),
			callable_mp_static(&mcp_tool_get_errors));

	// Editor tools.
	p_server->register_tool("get_editor_state", "Get current editor state (selected node, open scene, etc.)",
			MCPSchema::object().build(),
			callable_mp_static(&mcp_tool_get_editor_state));

	p_server->register_tool("take_screenshot", "Capture a screenshot of the editor/game window",
			MCPSchema::object().build(),
			callable_mp_static(&mcp_tool_take_screenshot));
}
