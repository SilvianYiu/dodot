/**************************************************************************/
/*  mcp_tools_codegen.cpp                                                 */
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

#include "mcp_tools_codegen.h"

#include "../mcp_server.h"
#include "../mcp_tool_helpers.h"

#include "core/object/class_db.h"

// ---------------------------------------------------------------------------
// generate_script_template
// ---------------------------------------------------------------------------

static Dictionary _tool_generate_script_template(const Dictionary &p_args) {
	String node_type = p_args.get("node_type", "Node");
	bool include_signals = p_args.get("include_signals", true);

	if (!ClassDB::class_exists(StringName(node_type))) {
		return mcp_error("Unknown class: " + node_type);
	}

	String script;
	script += "extends " + node_type + "\n\n";

	// Gather signals for the class if requested.
	if (include_signals) {
		List<MethodInfo> signal_list;
		ClassDB::get_signal_list(StringName(node_type), &signal_list, true);
		if (!signal_list.is_empty()) {
			script += "# --- Signals ---\n\n";
			for (const MethodInfo &si : signal_list) {
				// Generate a connected handler stub.
				String handler = "func _on_" + String(si.name) + "(";
				for (int i = 0; i < si.arguments.size(); i++) {
					if (i > 0) {
						handler += ", ";
					}
					handler += si.arguments[i].name + ": " + Variant::get_type_name(si.arguments[i].type);
				}
				handler += ") -> void:\n\tpass\n\n";
				script += handler;
			}
		}
	}

	// Add common lifecycle methods based on the class.
	script += "# --- Lifecycle ---\n\n";

	script += "func _ready() -> void:\n\tpass\n\n";
	script += "func _process(delta: float) -> void:\n\tpass\n";

	// Add _physics_process for physics-related nodes.
	if (ClassDB::is_parent_class(StringName(node_type), "CharacterBody2D") ||
			ClassDB::is_parent_class(StringName(node_type), "CharacterBody3D") ||
			ClassDB::is_parent_class(StringName(node_type), "RigidBody2D") ||
			ClassDB::is_parent_class(StringName(node_type), "RigidBody3D") ||
			node_type == "CharacterBody2D" || node_type == "CharacterBody3D" ||
			node_type == "RigidBody2D" || node_type == "RigidBody3D") {
		script += "\nfunc _physics_process(delta: float) -> void:\n\tpass\n";
	}

	// Add _input for Control nodes.
	if (ClassDB::is_parent_class(StringName(node_type), "Control") || node_type == "Control") {
		script += "\nfunc _input(event: InputEvent) -> void:\n\tpass\n";
	}

	Dictionary result = mcp_success();
	result["script"] = script;
	result["node_type"] = node_type;
	return result;
}

// ---------------------------------------------------------------------------
// get_available_signals
// ---------------------------------------------------------------------------

static Dictionary _tool_get_available_signals(const Dictionary &p_args) {
	String class_name = p_args.get("node_path_or_type", "");

	if (class_name.is_empty()) {
		return mcp_error("node_path_or_type is required");
	}

	// First try as a class name.
	StringName sn_class = StringName(class_name);
	if (!ClassDB::class_exists(sn_class)) {
		// Try to resolve as a node path.
		Dictionary err;
		Node *node = mcp_get_node(class_name, err);
		if (!node) {
			return mcp_error("Not a valid class name or node path: " + class_name);
		}
		sn_class = node->get_class_name();
	}

	List<MethodInfo> signal_list;
	ClassDB::get_signal_list(sn_class, &signal_list, true);

	Array signals;
	for (const MethodInfo &si : signal_list) {
		Dictionary sig;
		sig["name"] = String(si.name);
		Array args;
		for (int i = 0; i < si.arguments.size(); i++) {
			Dictionary arg;
			arg["name"] = si.arguments[i].name;
			arg["type"] = Variant::get_type_name(si.arguments[i].type);
			args.push_back(arg);
		}
		sig["arguments"] = args;
		signals.push_back(sig);
	}

	Dictionary result = mcp_success();
	result["class"] = String(sn_class);
	result["signals"] = signals;
	result["count"] = signals.size();
	return result;
}

// ---------------------------------------------------------------------------
// get_class_properties
// ---------------------------------------------------------------------------

static Dictionary _tool_get_class_properties(const Dictionary &p_args) {
	String class_name = p_args.get("class_name", "");

	if (class_name.is_empty()) {
		return mcp_error("class_name is required");
	}

	if (!ClassDB::class_exists(StringName(class_name))) {
		return mcp_error("Unknown class: " + class_name);
	}

	List<PropertyInfo> prop_list;
	ClassDB::get_property_list(StringName(class_name), &prop_list, true);

	Array properties;
	for (const PropertyInfo &pi : prop_list) {
		if (!(pi.usage & PROPERTY_USAGE_EDITOR)) {
			continue;
		}
		Dictionary prop;
		prop["name"] = pi.name;
		prop["type"] = Variant::get_type_name(pi.type);
		prop["hint"] = pi.hint;
		prop["hint_string"] = pi.hint_string;
		prop["usage"] = pi.usage;
		properties.push_back(prop);
	}

	Dictionary result = mcp_success();
	result["class"] = class_name;
	result["properties"] = properties;
	result["count"] = properties.size();
	return result;
}

// ---------------------------------------------------------------------------
// get_class_methods
// ---------------------------------------------------------------------------

static Dictionary _tool_get_class_methods(const Dictionary &p_args) {
	String class_name = p_args.get("class_name", "");

	if (class_name.is_empty()) {
		return mcp_error("class_name is required");
	}

	if (!ClassDB::class_exists(StringName(class_name))) {
		return mcp_error("Unknown class: " + class_name);
	}

	List<MethodInfo> method_list;
	ClassDB::get_method_list(StringName(class_name), &method_list, true);

	Array methods;
	for (const MethodInfo &mi : method_list) {
		Dictionary method;
		method["name"] = String(mi.name);
		method["return_type"] = Variant::get_type_name(mi.return_val.type);
		method["flags"] = mi.flags;

		Array args;
		for (int i = 0; i < mi.arguments.size(); i++) {
			Dictionary arg;
			arg["name"] = mi.arguments[i].name;
			arg["type"] = Variant::get_type_name(mi.arguments[i].type);
			args.push_back(arg);
		}
		method["arguments"] = args;
		methods.push_back(method);
	}

	Dictionary result = mcp_success();
	result["class"] = class_name;
	result["methods"] = methods;
	result["count"] = methods.size();
	return result;
}

// ---------------------------------------------------------------------------
// get_class_hierarchy
// ---------------------------------------------------------------------------

static Dictionary _tool_get_class_hierarchy(const Dictionary &p_args) {
	String class_name = p_args.get("class_name", "");

	if (class_name.is_empty()) {
		return mcp_error("class_name is required");
	}

	if (!ClassDB::class_exists(StringName(class_name))) {
		return mcp_error("Unknown class: " + class_name);
	}

	Array hierarchy;
	StringName current = StringName(class_name);
	while (current != StringName()) {
		hierarchy.push_back(String(current));
		current = ClassDB::get_parent_class(current);
	}

	Dictionary result = mcp_success();
	result["class"] = class_name;
	result["hierarchy"] = hierarchy;
	result["depth"] = hierarchy.size();
	return result;
}

// ---------------------------------------------------------------------------
// Registration
// ---------------------------------------------------------------------------

void mcp_register_codegen_tools(MCPServer *p_server) {
	// generate_script_template
	{
		Dictionary schema = MCPSchema::object()
									.prop("node_type", "string", "Godot node class name to generate a script for (e.g., CharacterBody2D)", true)
									.prop_bool("include_signals", "Whether to include signal handler stubs (default: true)")
									.build();

		p_server->register_tool("generate_script_template",
				"Generate a GDScript template for a given node type with lifecycle methods and optional signal handlers",
				schema, callable_mp_static(_tool_generate_script_template));
	}

	// get_available_signals
	{
		Dictionary schema = MCPSchema::object()
									.prop("node_path_or_type", "string", "Class name (e.g., Button) or node path (e.g., /root/Main/Player)", true)
									.build();

		p_server->register_tool("get_available_signals",
				"Get all signals available on a Godot class or scene node",
				schema, callable_mp_static(_tool_get_available_signals));
	}

	// get_class_properties
	{
		Dictionary schema = MCPSchema::object()
									.prop("class_name", "string", "Godot class name (e.g., Node2D, Camera3D)", true)
									.build();

		p_server->register_tool("get_class_properties",
				"Get all editor-visible properties of a Godot class",
				schema, callable_mp_static(_tool_get_class_properties));
	}

	// get_class_methods
	{
		Dictionary schema = MCPSchema::object()
									.prop("class_name", "string", "Godot class name (e.g., Node2D, Camera3D)", true)
									.build();

		p_server->register_tool("get_class_methods",
				"Get all methods of a Godot class with argument and return type information",
				schema, callable_mp_static(_tool_get_class_methods));
	}

	// get_class_hierarchy
	{
		Dictionary schema = MCPSchema::object()
									.prop("class_name", "string", "Godot class name (e.g., CharacterBody3D)", true)
									.build();

		p_server->register_tool("get_class_hierarchy",
				"Get the full inheritance chain for a Godot class from child to root",
				schema, callable_mp_static(_tool_get_class_hierarchy));
	}
}
