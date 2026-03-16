/**************************************************************************/
/*  mcp_tools_api_docs.cpp                                                */
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

#include "mcp_tools_api_docs.h"

#include "../mcp_server.h"
#include "../mcp_tool_helpers.h"

#include "core/object/class_db.h"

// ---------------------------------------------------------------------------
// Helper: build class documentation dictionary
// ---------------------------------------------------------------------------

static Dictionary _build_class_doc(const StringName &p_class) {
	Dictionary cls;
	cls["name"] = String(p_class);
	cls["parent"] = String(ClassDB::get_parent_class(p_class));

	// Properties.
	Array properties;
	{
		List<PropertyInfo> prop_list;
		ClassDB::get_property_list(p_class, &prop_list, true);
		for (const PropertyInfo &pi : prop_list) {
			if (!(pi.usage & PROPERTY_USAGE_EDITOR)) {
				continue;
			}
			Dictionary prop;
			prop["name"] = pi.name;
			prop["type"] = Variant::get_type_name(pi.type);
			prop["hint"] = pi.hint;
			prop["hint_string"] = pi.hint_string;
			properties.push_back(prop);
		}
	}
	cls["properties"] = properties;

	// Methods.
	Array methods;
	{
		List<MethodInfo> method_list;
		ClassDB::get_method_list(p_class, &method_list, true);
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
				if (i < mi.default_arguments.size()) {
					arg["default"] = mi.default_arguments[i];
				}
				args.push_back(arg);
			}
			method["arguments"] = args;
			methods.push_back(method);
		}
	}
	cls["methods"] = methods;

	// Signals.
	Array signals;
	{
		List<MethodInfo> signal_list;
		ClassDB::get_signal_list(p_class, &signal_list, true);
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
	}
	cls["signals"] = signals;

	// Integer constants.
	Array constants;
	{
		List<String> const_list;
		ClassDB::get_integer_constant_list(p_class, &const_list, true);
		for (const String &const_name : const_list) {
			Dictionary c;
			c["name"] = const_name;
			c["value"] = ClassDB::get_integer_constant(p_class, StringName(const_name));
			constants.push_back(c);
		}
	}
	cls["constants"] = constants;

	// Enums.
	Array enums;
	{
		List<StringName> enum_list;
		ClassDB::get_enum_list(p_class, &enum_list, true);
		for (const StringName &enum_name : enum_list) {
			Dictionary e;
			e["name"] = String(enum_name);

			List<StringName> enum_constants;
			ClassDB::get_enum_constants(p_class, enum_name, &enum_constants, true);
			Array values;
			for (const StringName &ec : enum_constants) {
				Dictionary ev;
				ev["name"] = String(ec);
				ev["value"] = ClassDB::get_integer_constant(p_class, ec);
				values.push_back(ev);
			}
			e["values"] = values;
			enums.push_back(e);
		}
	}
	cls["enums"] = enums;

	return cls;
}

// ---------------------------------------------------------------------------
// dump_api_json
// ---------------------------------------------------------------------------

static Dictionary _tool_dump_api_json(const Dictionary &p_args) {
	String classes_filter = p_args.get("classes_filter", "");

	LocalVector<StringName> class_list;
	ClassDB::get_class_list(class_list);

	// Build filter set.
	HashSet<String> filter_set;
	if (!classes_filter.is_empty()) {
		PackedStringArray filters = classes_filter.split(",");
		for (int i = 0; i < filters.size(); i++) {
			filter_set.insert(filters[i].strip_edges());
		}
	}

	Array classes;
	for (const StringName &cls_name : class_list) {
		if (!filter_set.is_empty() && !filter_set.has(String(cls_name))) {
			continue;
		}
		classes.push_back(_build_class_doc(cls_name));
	}

	Dictionary result = mcp_success();
	result["classes"] = classes;
	result["count"] = classes.size();
	return result;
}

// ---------------------------------------------------------------------------
// get_class_doc
// ---------------------------------------------------------------------------

static Dictionary _tool_get_class_doc(const Dictionary &p_args) {
	String class_name = p_args.get("class_name", "");

	if (class_name.is_empty()) {
		return mcp_error("class_name is required");
	}

	if (!ClassDB::class_exists(StringName(class_name))) {
		return mcp_error("Unknown class: " + class_name);
	}

	Dictionary cls = _build_class_doc(StringName(class_name));

	Dictionary result = mcp_success();
	result["class"] = cls;
	return result;
}

// ---------------------------------------------------------------------------
// Registration
// ---------------------------------------------------------------------------

void mcp_register_api_docs_tools(MCPServer *p_server) {
	// dump_api_json
	{
		Dictionary schema = MCPSchema::object()
									.prop("classes_filter", "string", "Comma-separated list of class names to include (empty for all classes)")
									.build();

		p_server->register_tool("dump_api_json",
				"Generate JSON API documentation for all or filtered Godot classes including properties, methods, signals, constants, and enums",
				schema, callable_mp_static(_tool_dump_api_json));
	}

	// get_class_doc
	{
		Dictionary schema = MCPSchema::object()
									.prop("class_name", "string", "Godot class name to get documentation for", true)
									.build();

		p_server->register_tool("get_class_doc",
				"Get complete API documentation for a single Godot class",
				schema, callable_mp_static(_tool_get_class_doc));
	}
}
