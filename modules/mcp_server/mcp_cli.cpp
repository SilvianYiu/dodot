/**************************************************************************/
/*  mcp_cli.cpp                                                           */
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

#include "mcp_cli.h"

#include "core/config/project_settings.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/io/json.h"
#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "core/object/class_db.h"
#include "core/object/script_language.h"
#include "core/version.h"
#include "scene/resources/packed_scene.h"

#include <cstdio>

void MCPCli::output_json(const String &p_json) {
	CharString utf8 = p_json.utf8();
	fprintf(stdout, "%s\n", utf8.get_data());
	fflush(stdout);
}

// =============================================================================
// Validate Scripts
// =============================================================================

int MCPCli::cmd_validate_scripts() {
	String project_path = ProjectSettings::get_singleton()->get_resource_path();

	Array results;
	int passed = 0;
	int failed = 0;
	int warnings = 0;

	// Find all .gd files.
	Vector<String> gd_files;
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
			} else if (fname.get_extension() == "gd") {
				gd_files.push_back(full_path);
			}
			fname = da->get_next();
		}
		da->list_dir_end();
	}

	for (const String &gd_path : gd_files) {
		String res_path = gd_path.replace(project_path, "res:/");
		if (!res_path.begins_with("res://")) {
			res_path = "res://" + res_path.substr(6);
		}

		Dictionary file_result;
		file_result["file"] = res_path;

		Ref<Script> script = ResourceLoader::load(res_path);
		if (script.is_valid()) {
			Error err = script->reload(true);
			if (err == OK) {
				file_result["status"] = "passed";
				passed++;
			} else {
				file_result["status"] = "failed";
				file_result["error"] = "Script validation failed";
				failed++;
			}
		} else {
			file_result["status"] = "error";
			file_result["error"] = "Failed to load script";
			failed++;
		}

		results.push_back(file_result);
	}

	Dictionary output;
	output["command"] = "validate-scripts";
	output["results"] = results;
	Dictionary summary;
	summary["total"] = gd_files.size();
	summary["passed"] = passed;
	summary["failed"] = failed;
	summary["warnings"] = warnings;
	output["summary"] = summary;
	output["success"] = failed == 0;

	output_json(JSON::stringify(output, "\t"));
	return failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}

// =============================================================================
// Lint GDScript
// =============================================================================

int MCPCli::cmd_lint_gdscript() {
	String project_path = ProjectSettings::get_singleton()->get_resource_path();

	Array all_diagnostics;
	int total_errors = 0;
	int total_warnings = 0;

	// Find all .gd files.
	Vector<String> gd_files;
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
			} else if (fname.get_extension() == "gd") {
				gd_files.push_back(full_path);
			}
			fname = da->get_next();
		}
		da->list_dir_end();
	}

	for (const String &gd_path : gd_files) {
		String res_path = gd_path.replace(project_path, "res:/");
		if (!res_path.begins_with("res://")) {
			res_path = "res://" + res_path.substr(6);
		}

		String source = FileAccess::get_file_as_string(gd_path);
		if (source.is_empty()) {
			continue;
		}

		Array file_diags;
		Vector<String> lines = source.split("\n");

		for (int i = 0; i < lines.size(); i++) {
			String line = lines[i].strip_edges();
			int line_num = i + 1;

			// UNTYPED_DECLARATION
			if (line.begins_with("var ") && !line.contains(":")) {
				Dictionary d;
				d["rule"] = "UNTYPED_DECLARATION";
				d["severity"] = "warning";
				d["line"] = line_num;
				d["message"] = "Variable has no type annotation";
				d["auto_fixable"] = false;
				file_diags.push_back(d);
				total_warnings++;
			}

			// MISSING_RETURN_TYPE
			if (line.begins_with("func ") && !line.contains("->") && line.contains("(")) {
				Dictionary d;
				d["rule"] = "MISSING_RETURN_TYPE";
				d["severity"] = "warning";
				d["line"] = line_num;
				d["message"] = "Function has no return type annotation";
				d["auto_fixable"] = false;
				file_diags.push_back(d);
				total_warnings++;
			}

			// SIGNAL_NAMING (check for non-snake_case signals)
			if (line.begins_with("signal ")) {
				String sig_name = line.substr(7);
				int paren = sig_name.find("(");
				if (paren >= 0) {
					sig_name = sig_name.substr(0, paren);
				}
				sig_name = sig_name.strip_edges();
				bool has_upper = false;
				for (int c = 0; c < sig_name.length(); c++) {
					if (sig_name[c] >= 'A' && sig_name[c] <= 'Z') {
						has_upper = true;
						break;
					}
				}
				if (has_upper) {
					Dictionary d;
					d["rule"] = "SIGNAL_NAMING";
					d["severity"] = "warning";
					d["line"] = line_num;
					d["message"] = "Signal '" + sig_name + "' should use snake_case";
					d["auto_fixable"] = true;
					file_diags.push_back(d);
					total_warnings++;
				}
			}

			// NODE_PATH_LITERAL
			if (line.contains("get_node(\"") || line.contains("$\"") || line.contains("$'")) {
				Dictionary d;
				d["rule"] = "NODE_PATH_LITERAL";
				d["severity"] = "info";
				d["line"] = line_num;
				d["message"] = "Hardcoded node path. Consider @export or %UniqueNode";
				d["auto_fixable"] = false;
				file_diags.push_back(d);
			}
		}

		// LONG_FUNCTION check
		bool in_func = false;
		int func_start = 0;
		String func_name;
		for (int i = 0; i < lines.size(); i++) {
			String line = lines[i];
			String stripped = line.strip_edges();
			if (stripped.begins_with("func ")) {
				if (in_func && (i - func_start) > 50) {
					Dictionary d;
					d["rule"] = "LONG_FUNCTION";
					d["severity"] = "warning";
					d["line"] = func_start + 1;
					d["message"] = "Function '" + func_name + "' is " + itos(i - func_start) + " lines long (>50)";
					d["auto_fixable"] = false;
					file_diags.push_back(d);
					total_warnings++;
				}
				in_func = true;
				func_start = i;
				func_name = stripped.substr(5, stripped.find("(") - 5).strip_edges();
			}
		}
		if (in_func && (lines.size() - func_start) > 50) {
			Dictionary d;
			d["rule"] = "LONG_FUNCTION";
			d["severity"] = "warning";
			d["line"] = func_start + 1;
			d["message"] = "Function '" + func_name + "' is " + itos(lines.size() - func_start) + " lines long (>50)";
			d["auto_fixable"] = false;
			file_diags.push_back(d);
			total_warnings++;
		}

		if (!file_diags.is_empty()) {
			Dictionary file_result;
			file_result["file"] = res_path;
			file_result["diagnostics"] = file_diags;
			all_diagnostics.push_back(file_result);
		}
	}

	Dictionary output;
	output["command"] = "lint-gdscript";
	output["files_scanned"] = gd_files.size();
	output["results"] = all_diagnostics;
	Dictionary summary;
	summary["total_errors"] = total_errors;
	summary["total_warnings"] = total_warnings;
	summary["files_with_issues"] = all_diagnostics.size();
	output["summary"] = summary;

	output_json(JSON::stringify(output, "\t"));
	return EXIT_SUCCESS;
}

// =============================================================================
// Dump API JSON
// =============================================================================

int MCPCli::cmd_dump_api_json(const String &p_output_path) {
	Array classes_arr;

	LocalVector<StringName> class_list;
	ClassDB::get_class_list(class_list);
	class_list.sort_custom<StringName::AlphCompare>();

	for (const StringName &class_name : class_list) {
		Dictionary cls;
		cls["name"] = String(class_name);
		cls["inherits"] = String(ClassDB::get_parent_class(class_name));

		// Properties.
		Array properties;
		List<PropertyInfo> prop_list;
		ClassDB::get_property_list(class_name, &prop_list, true);
		for (const PropertyInfo &pi : prop_list) {
			if (pi.usage & PROPERTY_USAGE_EDITOR || pi.usage & PROPERTY_USAGE_STORAGE) {
				Dictionary prop;
				prop["name"] = pi.name;
				prop["type"] = Variant::get_type_name(pi.type);
				prop["hint"] = pi.hint;
				properties.push_back(prop);
			}
		}
		cls["properties"] = properties;

		// Methods.
		Array methods;
		List<MethodInfo> method_list;
		ClassDB::get_method_list(class_name, &method_list, true);
		for (const MethodInfo &mi : method_list) {
			Dictionary method;
			method["name"] = mi.name;
			method["return_type"] = Variant::get_type_name(mi.return_val.type);

			Array args;
			for (const PropertyInfo &arg : mi.arguments) {
				Dictionary a;
				a["name"] = arg.name;
				a["type"] = Variant::get_type_name(arg.type);
				args.push_back(a);
			}
			method["arguments"] = args;
			methods.push_back(method);
		}
		cls["methods"] = methods;

		// Signals.
		Array signals_arr;
		List<MethodInfo> signal_list;
		ClassDB::get_signal_list(class_name, &signal_list, true);
		for (const MethodInfo &si : signal_list) {
			Dictionary sig;
			sig["name"] = si.name;
			Array sig_args;
			for (const PropertyInfo &arg : si.arguments) {
				Dictionary a;
				a["name"] = arg.name;
				a["type"] = Variant::get_type_name(arg.type);
				sig_args.push_back(a);
			}
			sig["arguments"] = sig_args;
			signals_arr.push_back(sig);
		}
		cls["signals"] = signals_arr;

		// Constants.
		Array constants;
		List<String> const_list;
		ClassDB::get_integer_constant_list(class_name, &const_list, true);
		for (const String &const_name : const_list) {
			Dictionary c;
			c["name"] = const_name;
			c["value"] = ClassDB::get_integer_constant(class_name, const_name);
			constants.push_back(c);
		}
		cls["constants"] = constants;

		classes_arr.push_back(cls);
	}

	Dictionary output;
	output["engine"] = "Godot Engine";
	output["version"] = String(VERSION_FULL_NAME);
	output["class_count"] = classes_arr.size();
	output["classes"] = classes_arr;

	String json_str = JSON::stringify(output, "\t");

	if (p_output_path == "-") {
		output_json(json_str);
	} else {
		Ref<FileAccess> fa = FileAccess::open(p_output_path, FileAccess::WRITE);
		if (fa.is_null()) {
			fprintf(stderr, "Error: Cannot write to %s\n", p_output_path.utf8().get_data());
			return EXIT_FAILURE;
		}
		fa->store_string(json_str);
	}

	Dictionary meta;
	meta["command"] = "dump-api-json";
	meta["output_path"] = p_output_path;
	meta["class_count"] = classes_arr.size();
	meta["success"] = true;
	if (p_output_path != "-") {
		output_json(JSON::stringify(meta));
	}

	return EXIT_SUCCESS;
}

// =============================================================================
// Project Info
// =============================================================================

int MCPCli::cmd_project_info() {
	ProjectSettings *ps = ProjectSettings::get_singleton();
	String project_path = ps->get_resource_path();

	Dictionary output;
	output["command"] = "project-info";

	Dictionary settings;
	settings["name"] = ps->get_setting("application/config/name", "");
	settings["main_scene"] = ps->get_setting("application/run/main_scene", "");
	settings["viewport_width"] = ps->get_setting("display/window/size/viewport_width", 1152);
	settings["viewport_height"] = ps->get_setting("display/window/size/viewport_height", 648);
	settings["renderer"] = ps->get_setting("rendering/renderer/rendering_method", "forward_plus");
	output["settings"] = settings;
	output["project_path"] = project_path;

	// Count files by type.
	int scene_count = 0, script_count = 0, resource_count = 0, other_count = 0;
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
			if (!fname.begins_with(".")) {
				String full_path = dir_path.path_join(fname);
				if (da->current_is_dir()) {
					dirs_to_scan.push_back(full_path);
				} else {
					String ext = fname.get_extension();
					if (ext == "tscn" || ext == "scn") {
						scene_count++;
					} else if (ext == "gd" || ext == "cs") {
						script_count++;
					} else if (ext == "tres" || ext == "res") {
						resource_count++;
					} else {
						other_count++;
					}
				}
			}
			fname = da->get_next();
		}
		da->list_dir_end();
	}

	Dictionary counts;
	counts["scenes"] = scene_count;
	counts["scripts"] = script_count;
	counts["resources"] = resource_count;
	counts["other"] = other_count;
	counts["total"] = scene_count + script_count + resource_count + other_count;
	output["file_counts"] = counts;

	output["engine_version"] = String(VERSION_FULL_NAME);
	output["success"] = true;

	output_json(JSON::stringify(output, "\t"));
	return EXIT_SUCCESS;
}

// =============================================================================
// List Files (shared helper)
// =============================================================================

static int _list_files_by_extension(const String &p_command, const Vector<String> &p_extensions) {
	String project_path = ProjectSettings::get_singleton()->get_resource_path();
	Array files;

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
			if (!fname.begins_with(".")) {
				String full_path = dir_path.path_join(fname);
				if (da->current_is_dir()) {
					dirs_to_scan.push_back(full_path);
				} else {
					String ext = fname.get_extension();
					bool match = p_extensions.is_empty();
					for (const String &e : p_extensions) {
						if (ext == e) {
							match = true;
							break;
						}
					}
					if (match) {
						String res_path = full_path.replace(project_path, "res:/");
						if (!res_path.begins_with("res://")) {
							res_path = "res://" + res_path.substr(6);
						}
						Dictionary f;
						f["path"] = res_path;
						f["name"] = fname;
						f["extension"] = ext;
						files.push_back(f);
					}
				}
			}
			fname = da->get_next();
		}
		da->list_dir_end();
	}

	Dictionary output;
	output["command"] = p_command;
	output["files"] = files;
	output["count"] = files.size();
	output["success"] = true;

	MCPCli::output_json(JSON::stringify(output, "\t"));
	return EXIT_SUCCESS;
}

int MCPCli::cmd_list_scenes() {
	Vector<String> exts;
	exts.push_back("tscn");
	exts.push_back("scn");
	return _list_files_by_extension("list-scenes", exts);
}

int MCPCli::cmd_list_scripts() {
	Vector<String> exts;
	exts.push_back("gd");
	exts.push_back("cs");
	return _list_files_by_extension("list-scripts", exts);
}

int MCPCli::cmd_list_resources() {
	Vector<String> exts; // Empty = all files.
	return _list_files_by_extension("list-resources", exts);
}

// =============================================================================
// Scene Info
// =============================================================================

int MCPCli::cmd_scene_info(const String &p_scene_path) {
	Ref<PackedScene> scene = ResourceLoader::load(p_scene_path);
	if (scene.is_null()) {
		Dictionary output;
		output["command"] = "scene-info";
		output["error"] = "Failed to load scene: " + p_scene_path;
		output["success"] = false;
		output_json(JSON::stringify(output));
		return EXIT_FAILURE;
	}

	SceneState *state = scene->get_state().ptr();
	if (!state) {
		Dictionary output;
		output["command"] = "scene-info";
		output["error"] = "Failed to get scene state";
		output["success"] = false;
		output_json(JSON::stringify(output));
		return EXIT_FAILURE;
	}

	Dictionary output;
	output["command"] = "scene-info";
	output["path"] = p_scene_path;
	output["node_count"] = state->get_node_count();

	Array nodes;
	for (int i = 0; i < state->get_node_count(); i++) {
		Dictionary node;
		node["name"] = String(state->get_node_name(i));
		node["type"] = String(state->get_node_type(i));
		NodePath parent = state->get_node_path(i, true);
		node["parent"] = String(parent);

		// Node properties.
		Dictionary props;
		for (int j = 0; j < state->get_node_property_count(i); j++) {
			String prop_name = state->get_node_property_name(i, j);
			Variant prop_value = state->get_node_property_value(i, j);
			// Convert to string for JSON safety.
			props[prop_name] = Variant::get_type_name(prop_value.get_type()) + ": " + String(prop_value);
		}
		node["properties"] = props;

		nodes.push_back(node);
	}
	output["nodes"] = nodes;

	// Connections.
	Array connections;
	for (int i = 0; i < state->get_connection_count(); i++) {
		Dictionary conn;
		conn["signal"] = String(state->get_connection_signal(i));
		conn["from"] = String(state->get_connection_source(i));
		conn["to"] = String(state->get_connection_target(i));
		conn["method"] = String(state->get_connection_method(i));
		connections.push_back(conn);
	}
	output["connections"] = connections;
	output["success"] = true;

	output_json(JSON::stringify(output, "\t"));
	return EXIT_SUCCESS;
}

// =============================================================================
// Script Info
// =============================================================================

int MCPCli::cmd_script_info(const String &p_script_path) {
	if (!FileAccess::exists(p_script_path)) {
		Dictionary output;
		output["command"] = "script-info";
		output["error"] = "File not found: " + p_script_path;
		output["success"] = false;
		output_json(JSON::stringify(output));
		return EXIT_FAILURE;
	}

	String source = FileAccess::get_file_as_string(p_script_path);

	Array functions;
	Array variables;
	Array signals_arr;
	Array classes;
	int line_count = 0;

	Vector<String> lines = source.split("\n");
	line_count = lines.size();

	for (int i = 0; i < lines.size(); i++) {
		String line = lines[i].strip_edges();

		if (line.begins_with("func ")) {
			Dictionary sym;
			int paren = line.find("(");
			sym["name"] = line.substr(5, paren >= 0 ? paren - 5 : line.length() - 5).strip_edges();
			sym["line"] = i + 1;
			sym["signature"] = line;
			// Check for return type.
			int arrow = line.find("->");
			if (arrow >= 0) {
				sym["return_type"] = line.substr(arrow + 2, line.find(":") > arrow ? line.find(":") - arrow - 2 : line.length() - arrow - 2).strip_edges();
			}
			functions.push_back(sym);
		} else if (line.begins_with("var ") || line.begins_with("@export var ") || line.begins_with("@onready var ")) {
			Dictionary sym;
			int var_pos = line.find("var ") + 4;
			String var_part = line.substr(var_pos);
			int colon = var_part.find(":");
			int eq = var_part.find("=");
			int end = (colon >= 0 && (eq < 0 || colon < eq)) ? colon : (eq >= 0 ? eq : var_part.length());
			sym["name"] = var_part.substr(0, end).strip_edges();
			sym["line"] = i + 1;
			sym["exported"] = line.begins_with("@export");
			sym["onready"] = line.begins_with("@onready");
			if (colon >= 0) {
				int type_end = eq >= 0 ? eq - colon - 1 : var_part.length() - colon - 1;
				sym["type"] = var_part.substr(colon + 1, type_end).strip_edges();
			}
			variables.push_back(sym);
		} else if (line.begins_with("signal ")) {
			Dictionary sym;
			int paren = line.find("(");
			sym["name"] = line.substr(7, (paren >= 0 ? paren : line.length()) - 7).strip_edges();
			sym["line"] = i + 1;
			if (paren >= 0) {
				int close = line.find(")");
				if (close > paren) {
					sym["parameters"] = line.substr(paren + 1, close - paren - 1).strip_edges();
				}
			}
			signals_arr.push_back(sym);
		} else if (line.begins_with("class ")) {
			Dictionary sym;
			int colon = line.find(":");
			sym["name"] = line.substr(6, (colon >= 0 ? colon : line.length()) - 6).strip_edges();
			sym["line"] = i + 1;
			classes.push_back(sym);
		}
	}

	// Get extends info.
	String extends_class;
	for (const String &line : lines) {
		String stripped = line.strip_edges();
		if (stripped.begins_with("extends ")) {
			extends_class = stripped.substr(8).strip_edges();
			break;
		}
	}

	Dictionary output;
	output["command"] = "script-info";
	output["path"] = p_script_path;
	output["extends"] = extends_class;
	output["line_count"] = line_count;
	output["functions"] = functions;
	output["variables"] = variables;
	output["signals"] = signals_arr;
	output["classes"] = classes;
	output["success"] = true;

	output_json(JSON::stringify(output, "\t"));
	return EXIT_SUCCESS;
}

// =============================================================================
// Export Scene to JSON
// =============================================================================

int MCPCli::cmd_export_scene_json(const String &p_source, const String &p_target) {
	Ref<PackedScene> scene = ResourceLoader::load(p_source);
	if (scene.is_null()) {
		Dictionary output;
		output["command"] = "export-scene-json";
		output["error"] = "Failed to load scene: " + p_source;
		output["success"] = false;
		output_json(JSON::stringify(output));
		return EXIT_FAILURE;
	}

	SceneState *state = scene->get_state().ptr();
	if (!state) {
		Dictionary output;
		output["command"] = "export-scene-json";
		output["error"] = "Failed to get scene state";
		output["success"] = false;
		output_json(JSON::stringify(output));
		return EXIT_FAILURE;
	}

	Dictionary scene_json;
	scene_json["format_version"] = 1;
	scene_json["type"] = "PackedScene";
	scene_json["source"] = p_source;

	// Nodes.
	Array nodes;
	for (int i = 0; i < state->get_node_count(); i++) {
		Dictionary node;
		node["name"] = String(state->get_node_name(i));
		node["type"] = String(state->get_node_type(i));
		node["parent"] = String(state->get_node_path(i, true));

		Dictionary props;
		for (int j = 0; j < state->get_node_property_count(i); j++) {
			String pname = state->get_node_property_name(i, j);
			Variant pval = state->get_node_property_value(i, j);
			props[pname] = String(pval);
		}
		if (!props.is_empty()) {
			node["properties"] = props;
		}
		nodes.push_back(node);
	}
	scene_json["nodes"] = nodes;

	// Connections.
	Array connections;
	for (int i = 0; i < state->get_connection_count(); i++) {
		Dictionary conn;
		conn["signal"] = String(state->get_connection_signal(i));
		conn["from"] = String(state->get_connection_source(i));
		conn["to"] = String(state->get_connection_target(i));
		conn["method"] = String(state->get_connection_method(i));
		connections.push_back(conn);
	}
	scene_json["connections"] = connections;

	String json_str = JSON::stringify(scene_json, "\t");

	if (p_target.is_empty() || p_target == "-") {
		output_json(json_str);
	} else {
		Ref<FileAccess> fa = FileAccess::open(p_target, FileAccess::WRITE);
		if (fa.is_null()) {
			Dictionary output;
			output["error"] = "Cannot write to: " + p_target;
			output["success"] = false;
			output_json(JSON::stringify(output));
			return EXIT_FAILURE;
		}
		fa->store_string(json_str);

		Dictionary output;
		output["command"] = "export-scene-json";
		output["source"] = p_source;
		output["target"] = p_target;
		output["node_count"] = state->get_node_count();
		output["success"] = true;
		output_json(JSON::stringify(output));
	}

	return EXIT_SUCCESS;
}

// =============================================================================
// Import Scene from JSON
// =============================================================================

int MCPCli::cmd_import_scene_json(const String &p_source, const String &p_target) {
	// Read JSON file.
	String json_str = FileAccess::get_file_as_string(p_source);
	if (json_str.is_empty()) {
		Dictionary output;
		output["command"] = "import-scene-json";
		output["error"] = "Failed to read: " + p_source;
		output["success"] = false;
		output_json(JSON::stringify(output));
		return EXIT_FAILURE;
	}

	Ref<JSON> json;
	json.instantiate();
	Error err = json->parse(json_str);
	if (err != OK || json->get_data().get_type() != Variant::DICTIONARY) {
		Dictionary output;
		output["command"] = "import-scene-json";
		output["error"] = "JSON parse error: " + json->get_error_message() + " at line " + itos(json->get_error_line());
		output["success"] = false;
		output_json(JSON::stringify(output));
		return EXIT_FAILURE;
	}

	// For now, output a message that full import requires scene tree.
	// A basic text-based .tscn generation is possible.
	Dictionary scene_data = json->get_data();
	Array nodes = scene_data.get("nodes", Array());

	// Generate .tscn text format.
	String tscn = "[gd_scene format=3]\n\n";

	for (int i = 0; i < nodes.size(); i++) {
		Dictionary node = nodes[i];
		String name = node.get("name", "Node");
		String type = node.get("type", "Node");
		String parent = node.get("parent", "");

		if (i == 0) {
			tscn += "[node name=\"" + name + "\" type=\"" + type + "\"]\n";
		} else {
			tscn += "\n[node name=\"" + name + "\" type=\"" + type + "\" parent=\"" + parent + "\"]\n";
		}

		Dictionary props = node.get("properties", Dictionary());
		for (const Variant *key = props.next(nullptr); key; key = props.next(key)) {
			tscn += String(*key) + " = " + String(props[*key]) + "\n";
		}
	}

	// Connections.
	Array connections = scene_data.get("connections", Array());
	for (int i = 0; i < connections.size(); i++) {
		Dictionary conn = connections[i];
		tscn += "\n[connection signal=\"" + String(conn.get("signal", "")) + "\" from=\"" + String(conn.get("from", "")) + "\" to=\"" + String(conn.get("to", "")) + "\" method=\"" + String(conn.get("method", "")) + "\"]\n";
	}

	String target = p_target.is_empty() ? p_source.get_basename() + ".tscn" : p_target;
	Ref<FileAccess> fa = FileAccess::open(target, FileAccess::WRITE);
	if (fa.is_null()) {
		Dictionary output;
		output["error"] = "Cannot write to: " + target;
		output["success"] = false;
		output_json(JSON::stringify(output));
		return EXIT_FAILURE;
	}
	fa->store_string(tscn);

	Dictionary output;
	output["command"] = "import-scene-json";
	output["source"] = p_source;
	output["target"] = target;
	output["node_count"] = nodes.size();
	output["success"] = true;
	output_json(JSON::stringify(output));
	return EXIT_SUCCESS;
}

// =============================================================================
// Query
// =============================================================================

int MCPCli::cmd_query(const String &p_expression) {
	Dictionary output;
	output["command"] = "query";
	output["expression"] = p_expression;

	// Parse query expressions.
	if (p_expression.begins_with("get_all_nodes_of_type(")) {
		// Extract type name.
		int start = p_expression.find("(") + 1;
		int end = p_expression.find(")");
		String type_name = p_expression.substr(start, end - start).strip_edges();

		// Search all .tscn files for nodes of this type.
		String project_path = ProjectSettings::get_singleton()->get_resource_path();
		Array matches;

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
				if (!fname.begins_with(".")) {
					String full_path = dir_path.path_join(fname);
					if (da->current_is_dir()) {
						dirs_to_scan.push_back(full_path);
					} else if (fname.get_extension() == "tscn" || fname.get_extension() == "scn") {
						Ref<PackedScene> scene = ResourceLoader::load(full_path);
						if (scene.is_valid() && scene->get_state().is_valid()) {
							SceneState *state = scene->get_state().ptr();
							for (int i = 0; i < state->get_node_count(); i++) {
								if (String(state->get_node_type(i)) == type_name) {
									String res_path = full_path.replace(project_path, "res:/");
									if (!res_path.begins_with("res://")) {
										res_path = "res://" + res_path.substr(6);
									}
									Dictionary match;
									match["scene"] = res_path;
									match["node_name"] = String(state->get_node_name(i));
									match["node_path"] = String(state->get_node_path(i, true));
									matches.push_back(match);
								}
							}
						}
					}
				}
				fname = da->get_next();
			}
			da->list_dir_end();
		}

		output["results"] = matches;
		output["count"] = matches.size();
		output["success"] = true;

	} else if (p_expression.begins_with("find_scripts_using(")) {
		int start = p_expression.find("(") + 1;
		int end = p_expression.find(")");
		String search_term = p_expression.substr(start, end - start).strip_edges();

		String project_path = ProjectSettings::get_singleton()->get_resource_path();
		Array matches;

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
				if (!fname.begins_with(".")) {
					String full_path = dir_path.path_join(fname);
					if (da->current_is_dir()) {
						dirs_to_scan.push_back(full_path);
					} else if (fname.get_extension() == "gd") {
						String content = FileAccess::get_file_as_string(full_path);
						if (content.contains(search_term)) {
							String res_path = full_path.replace(project_path, "res:/");
							if (!res_path.begins_with("res://")) {
								res_path = "res://" + res_path.substr(6);
							}

							// Find matching lines.
							Array line_matches;
							Vector<String> lines = content.split("\n");
							for (int i = 0; i < lines.size(); i++) {
								if (lines[i].contains(search_term)) {
									Dictionary lm;
									lm["line"] = i + 1;
									lm["content"] = lines[i].strip_edges();
									line_matches.push_back(lm);
								}
							}

							Dictionary match;
							match["file"] = res_path;
							match["matches"] = line_matches;
							matches.push_back(match);
						}
					}
				}
				fname = da->get_next();
			}
			da->list_dir_end();
		}

		output["results"] = matches;
		output["count"] = matches.size();
		output["success"] = true;

	} else if (p_expression.begins_with("find_class(")) {
		int start = p_expression.find("(") + 1;
		int end = p_expression.find(")");
		String class_name = p_expression.substr(start, end - start).strip_edges();

		if (ClassDB::class_exists(class_name)) {
			Dictionary class_info;
			class_info["name"] = class_name;
			class_info["inherits"] = String(ClassDB::get_parent_class(class_name));
			class_info["instantiable"] = ClassDB::can_instantiate(class_name);

			// Get immediate properties (not inherited).
			Array props;
			List<PropertyInfo> prop_list;
			ClassDB::get_property_list(class_name, &prop_list, true);
			for (const PropertyInfo &pi : prop_list) {
				if (pi.usage & PROPERTY_USAGE_EDITOR) {
					Dictionary p;
					p["name"] = pi.name;
					p["type"] = Variant::get_type_name(pi.type);
					props.push_back(p);
				}
			}
			class_info["properties"] = props;

			output["result"] = class_info;
			output["success"] = true;
		} else {
			output["error"] = "Class not found: " + class_name;
			output["success"] = false;
		}

	} else if (p_expression == "list_node_types") {
		Array types;
		LocalVector<StringName> class_list;
		ClassDB::get_class_list(class_list);
		for (const StringName &cn : class_list) {
			if (ClassDB::is_parent_class(cn, "Node") && ClassDB::can_instantiate(cn)) {
				types.push_back(String(cn));
			}
		}
		types.sort();
		output["results"] = types;
		output["count"] = types.size();
		output["success"] = true;

	} else if (p_expression == "list_resource_types") {
		Array types;
		LocalVector<StringName> class_list;
		ClassDB::get_class_list(class_list);
		for (const StringName &cn : class_list) {
			if (ClassDB::is_parent_class(cn, "Resource") && ClassDB::can_instantiate(cn)) {
				types.push_back(String(cn));
			}
		}
		types.sort();
		output["results"] = types;
		output["count"] = types.size();
		output["success"] = true;

	} else {
		output["error"] = "Unknown query expression. Supported: get_all_nodes_of_type(Type), find_scripts_using(term), find_class(ClassName), list_node_types, list_resource_types";
		output["success"] = false;
	}

	output_json(JSON::stringify(output, "\t"));
	return output.get("success", false) ? EXIT_SUCCESS : EXIT_FAILURE;
}
