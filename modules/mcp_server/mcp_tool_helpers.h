/**************************************************************************/
/*  mcp_tool_helpers.h                                                    */
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

#include "core/object/callable_mp.h"
#include "core/object/class_db.h"
#include "core/variant/dictionary.h"
#include "core/variant/array.h"
#include "core/string/ustring.h"
#include "scene/main/node.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"

// --- Error / Success helpers ---

static inline Dictionary mcp_error(const String &p_message) {
	Dictionary result;
	result["error"] = p_message;
	return result;
}

static inline Dictionary mcp_success() {
	Dictionary result;
	result["success"] = true;
	return result;
}

static inline Dictionary mcp_success(const Dictionary &p_data) {
	Dictionary result = p_data.duplicate();
	result["success"] = true;
	return result;
}

// --- SceneTree / Node helpers ---

static inline SceneTree *mcp_get_scene_tree() {
	return SceneTree::get_singleton();
}

static inline Node *mcp_get_node(const String &p_path, Dictionary &r_error) {
	SceneTree *st = SceneTree::get_singleton();
	if (!st) {
		r_error = mcp_error("No SceneTree available");
		return nullptr;
	}
	Node *root = st->get_root();
	if (!root) {
		r_error = mcp_error("No root node");
		return nullptr;
	}
	Node *node = root->get_node_or_null(NodePath(p_path));
	if (!node) {
		r_error = mcp_error("Node not found: " + p_path);
		return nullptr;
	}
	return node;
}

static inline Node *mcp_get_or_create_parent(const String &p_parent_path, Dictionary &r_error) {
	return mcp_get_node(p_parent_path, r_error);
}

// --- Schema builder ---

class MCPSchema {
	Dictionary schema;
	Dictionary props;
	Array required_list;

public:
	MCPSchema() {
		schema["type"] = "object";
	}

	static MCPSchema object() {
		return MCPSchema();
	}

	MCPSchema &prop(const String &p_name, const String &p_type, const String &p_desc, bool p_required = false) {
		Dictionary p;
		p["type"] = p_type;
		p["description"] = p_desc;
		props[p_name] = p;
		if (p_required) {
			required_list.push_back(p_name);
		}
		return *this;
	}

	MCPSchema &prop_enum(const String &p_name, const Array &p_values, const String &p_desc, bool p_required = false) {
		Dictionary p;
		p["type"] = "string";
		p["description"] = p_desc;
		p["enum"] = p_values;
		props[p_name] = p;
		if (p_required) {
			required_list.push_back(p_name);
		}
		return *this;
	}

	MCPSchema &prop_number(const String &p_name, const String &p_desc, bool p_required = false) {
		Dictionary p;
		p["type"] = "number";
		p["description"] = p_desc;
		props[p_name] = p;
		if (p_required) {
			required_list.push_back(p_name);
		}
		return *this;
	}

	MCPSchema &prop_bool(const String &p_name, const String &p_desc, bool p_required = false) {
		Dictionary p;
		p["type"] = "boolean";
		p["description"] = p_desc;
		props[p_name] = p;
		if (p_required) {
			required_list.push_back(p_name);
		}
		return *this;
	}

	MCPSchema &prop_object(const String &p_name, const String &p_desc, bool p_required = false) {
		Dictionary p;
		p["type"] = "object";
		p["description"] = p_desc;
		props[p_name] = p;
		if (p_required) {
			required_list.push_back(p_name);
		}
		return *this;
	}

	MCPSchema &prop_array(const String &p_name, const String &p_desc, bool p_required = false) {
		Dictionary p;
		p["type"] = "array";
		p["description"] = p_desc;
		props[p_name] = p;
		if (p_required) {
			required_list.push_back(p_name);
		}
		return *this;
	}

	Dictionary build() {
		schema["properties"] = props;
		if (!required_list.is_empty()) {
			schema["required"] = required_list;
		}
		return schema;
	}
};

// --- Node creation helper ---

static inline Node *mcp_create_node_of_type(const String &p_type, const String &p_name, Dictionary &r_error) {
	Object *obj = ClassDB::instantiate(p_type);
	Node *node = Object::cast_to<Node>(obj);
	if (!node) {
		if (obj) {
			memdelete(obj);
		}
		r_error = mcp_error("Failed to create node of type: " + p_type);
		return nullptr;
	}
	if (!p_name.is_empty()) {
		node->set_name(p_name);
	}
	return node;
}

static inline void mcp_add_child_to_scene(Node *p_parent, Node *p_child) {
	p_parent->add_child(p_child, true);
	SceneTree *st = SceneTree::get_singleton();
	if (st) {
		Node *edited = st->get_edited_scene_root();
		p_child->set_owner(edited ? edited : st->get_root());
	}
}

// --- Property helpers ---

static inline void mcp_set_properties(Node *p_node, const Dictionary &p_properties) {
	for (const Variant *key = p_properties.next(nullptr); key; key = p_properties.next(key)) {
		p_node->set(StringName(*key), p_properties[*key]);
	}
}

static inline Dictionary mcp_get_node_info(Node *p_node) {
	Dictionary d;
	d["name"] = p_node->get_name();
	d["type"] = p_node->get_class();
	d["path"] = String(p_node->get_path());
	return d;
}
