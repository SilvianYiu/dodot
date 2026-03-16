/**************************************************************************/
/*  mcp_tools_3d_transform.cpp                                            */
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

#include "mcp_tools_3d_transform.h"

#include "../mcp_server.h"
#include "../mcp_tool_helpers.h"

#include "scene/3d/node_3d.h"

// ---------------------------------------------------------------------------
// Helper: get Node3D from path
// ---------------------------------------------------------------------------

static Node3D *_get_node_3d(const String &p_path, Dictionary &r_error) {
	Node *node = mcp_get_node(p_path, r_error);
	if (!node) {
		return nullptr;
	}
	Node3D *node_3d = Object::cast_to<Node3D>(node);
	if (!node_3d) {
		r_error = mcp_error("Node is not a Node3D: " + p_path);
		return nullptr;
	}
	return node_3d;
}

static Vector3 _dict_to_vector3(const Dictionary &p_dict, const Vector3 &p_default = Vector3()) {
	Vector3 v = p_default;
	if (p_dict.has("x")) {
		v.x = p_dict["x"];
	}
	if (p_dict.has("y")) {
		v.y = p_dict["y"];
	}
	if (p_dict.has("z")) {
		v.z = p_dict["z"];
	}
	return v;
}

static Dictionary _vector3_to_dict(const Vector3 &p_vec) {
	Dictionary d;
	d["x"] = p_vec.x;
	d["y"] = p_vec.y;
	d["z"] = p_vec.z;
	return d;
}

// ---------------------------------------------------------------------------
// set_transform
// ---------------------------------------------------------------------------

static Dictionary _tool_set_transform(const Dictionary &p_args) {
	String node_path = p_args.get("node_path", "");

	if (node_path.is_empty()) {
		return mcp_error("node_path is required");
	}

	Dictionary err;
	Node3D *node = _get_node_3d(node_path, err);
	if (!node) {
		return err;
	}

	if (p_args.has("position")) {
		Dictionary pos = p_args["position"];
		node->set_position(_dict_to_vector3(pos, node->get_position()));
	}

	if (p_args.has("rotation")) {
		Dictionary rot = p_args["rotation"];
		node->set_rotation_degrees(_dict_to_vector3(rot, node->get_rotation_degrees()));
	}

	if (p_args.has("scale")) {
		Dictionary scl = p_args["scale"];
		node->set_scale(_dict_to_vector3(scl, node->get_scale()));
	}

	Dictionary result = mcp_success();
	result["node_path"] = node_path;
	result["position"] = _vector3_to_dict(node->get_position());
	result["rotation_degrees"] = _vector3_to_dict(node->get_rotation_degrees());
	result["scale"] = _vector3_to_dict(node->get_scale());
	return result;
}

// ---------------------------------------------------------------------------
// translate_node
// ---------------------------------------------------------------------------

static Dictionary _tool_translate_node(const Dictionary &p_args) {
	String node_path = p_args.get("node_path", "");
	Dictionary offset = p_args.get("offset", Dictionary());

	if (node_path.is_empty()) {
		return mcp_error("node_path is required");
	}

	Dictionary err;
	Node3D *node = _get_node_3d(node_path, err);
	if (!node) {
		return err;
	}

	Vector3 translation = _dict_to_vector3(offset);
	node->set_position(node->get_position() + translation);

	Dictionary result = mcp_success();
	result["node_path"] = node_path;
	result["new_position"] = _vector3_to_dict(node->get_position());
	return result;
}

// ---------------------------------------------------------------------------
// rotate_node
// ---------------------------------------------------------------------------

static Dictionary _tool_rotate_node(const Dictionary &p_args) {
	String node_path = p_args.get("node_path", "");
	Dictionary axis_dict = p_args.get("axis", Dictionary());
	double angle_degrees = p_args.get("angle_degrees", 0.0);

	if (node_path.is_empty()) {
		return mcp_error("node_path is required");
	}

	Dictionary err;
	Node3D *node = _get_node_3d(node_path, err);
	if (!node) {
		return err;
	}

	Vector3 axis = _dict_to_vector3(axis_dict, Vector3(0, 1, 0));
	if (axis.length_squared() < CMP_EPSILON) {
		return mcp_error("axis must be a non-zero vector");
	}
	axis.normalize();

	double angle_radians = Math::deg_to_rad(angle_degrees);
	node->rotate(axis, angle_radians);

	Dictionary result = mcp_success();
	result["node_path"] = node_path;
	result["rotation_degrees"] = _vector3_to_dict(node->get_rotation_degrees());
	return result;
}

// ---------------------------------------------------------------------------
// look_at_target
// ---------------------------------------------------------------------------

static Dictionary _tool_look_at_target(const Dictionary &p_args) {
	String node_path = p_args.get("node_path", "");
	Dictionary target_dict = p_args.get("target", Dictionary());
	Dictionary up_dict = p_args.get("up", Dictionary());

	if (node_path.is_empty()) {
		return mcp_error("node_path is required");
	}

	Dictionary err;
	Node3D *node = _get_node_3d(node_path, err);
	if (!node) {
		return err;
	}

	Vector3 target = _dict_to_vector3(target_dict);
	Vector3 up = _dict_to_vector3(up_dict, Vector3(0, 1, 0));

	if (up.length_squared() < CMP_EPSILON) {
		up = Vector3(0, 1, 0);
	}

	node->look_at(target, up);

	Dictionary result = mcp_success();
	result["node_path"] = node_path;
	result["rotation_degrees"] = _vector3_to_dict(node->get_rotation_degrees());
	return result;
}

// ---------------------------------------------------------------------------
// get_transform
// ---------------------------------------------------------------------------

static Dictionary _tool_get_transform(const Dictionary &p_args) {
	String node_path = p_args.get("node_path", "");

	if (node_path.is_empty()) {
		return mcp_error("node_path is required");
	}

	Dictionary err;
	Node3D *node = _get_node_3d(node_path, err);
	if (!node) {
		return err;
	}

	Dictionary result = mcp_success();
	result["node_path"] = node_path;
	result["position"] = _vector3_to_dict(node->get_position());
	result["rotation_degrees"] = _vector3_to_dict(node->get_rotation_degrees());
	result["scale"] = _vector3_to_dict(node->get_scale());

	// Also include global transform info.
	result["global_position"] = _vector3_to_dict(node->get_global_position());
	result["global_rotation_degrees"] = _vector3_to_dict(node->get_global_rotation_degrees());

	return result;
}

// ---------------------------------------------------------------------------
// Registration
// ---------------------------------------------------------------------------

void mcp_register_3d_transform_tools(MCPServer *p_server) {
	// set_transform
	{
		Dictionary schema = MCPSchema::object()
									.prop("node_path", "string", "Path to the Node3D node", true)
									.prop_object("position", "Position as {x, y, z}")
									.prop_object("rotation", "Rotation in degrees as {x, y, z}")
									.prop_object("scale", "Scale as {x, y, z}")
									.build();

		p_server->register_tool("set_transform",
				"Set the full transform (position, rotation, scale) of a Node3D",
				schema, callable_mp_static(_tool_set_transform));
	}

	// translate_node
	{
		Dictionary schema = MCPSchema::object()
									.prop("node_path", "string", "Path to the Node3D node", true)
									.prop_object("offset", "Translation offset as {x, y, z}", true)
									.build();

		p_server->register_tool("translate_node",
				"Translate a Node3D by an offset vector",
				schema, callable_mp_static(_tool_translate_node));
	}

	// rotate_node
	{
		Dictionary schema = MCPSchema::object()
									.prop("node_path", "string", "Path to the Node3D node", true)
									.prop_object("axis", "Rotation axis as {x, y, z} (default: {0, 1, 0} for Y-up)", true)
									.prop_number("angle_degrees", "Rotation angle in degrees", true)
									.build();

		p_server->register_tool("rotate_node",
				"Rotate a Node3D around an axis by a given angle in degrees",
				schema, callable_mp_static(_tool_rotate_node));
	}

	// look_at_target
	{
		Dictionary schema = MCPSchema::object()
									.prop("node_path", "string", "Path to the Node3D node", true)
									.prop_object("target", "Target position to look at as {x, y, z}", true)
									.prop_object("up", "Up vector as {x, y, z} (default: {0, 1, 0})")
									.build();

		p_server->register_tool("look_at_target",
				"Make a Node3D look at a target position",
				schema, callable_mp_static(_tool_look_at_target));
	}

	// get_transform
	{
		Dictionary schema = MCPSchema::object()
									.prop("node_path", "string", "Path to the Node3D node", true)
									.build();

		p_server->register_tool("get_transform",
				"Get the current transform (position, rotation, scale) of a Node3D, including global transform",
				schema, callable_mp_static(_tool_get_transform));
	}
}
