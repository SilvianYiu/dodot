/**************************************************************************/
/*  mcp_tools_runtime.cpp                                                 */
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

#include "mcp_tools_runtime.h"

#include "../mcp_server.h"
#include "../mcp_tool_helpers.h"

#include "core/variant/variant.h"
#include "main/performance.h"
#include "scene/3d/physics/character_body_3d.h"
#include "scene/3d/physics/physics_body_3d.h"
#include "scene/3d/physics/rigid_body_3d.h"

// --- get_node_runtime_state ---

static Dictionary mcp_tool_get_node_runtime_state(const Dictionary &p_args) {
	String node_path = p_args.get("node_path", "");
	if (node_path.is_empty()) {
		return mcp_error("node_path is required");
	}

	Dictionary err;
	Node *node = mcp_get_node(node_path, err);
	if (!node) {
		return err;
	}

	Dictionary result;
	result["success"] = true;
	result["node_path"] = String(node->get_path());
	result["class"] = node->get_class();

	// Iterate all properties.
	List<PropertyInfo> props;
	node->get_property_list(&props);

	Dictionary properties;
	for (const PropertyInfo &pi : props) {
		if (pi.name.is_empty()) {
			continue;
		}
		// Skip internal/editor properties that start with underscore.
		if (pi.usage & PROPERTY_USAGE_STORAGE || pi.usage & PROPERTY_USAGE_EDITOR) {
			Variant val = node->get(pi.name);
			properties[pi.name] = val.stringify();
		}
	}
	result["properties"] = properties;

	return result;
}

// --- call_node_method ---

static Dictionary mcp_tool_call_node_method(const Dictionary &p_args) {
	String node_path = p_args.get("node_path", "");
	if (node_path.is_empty()) {
		return mcp_error("node_path is required");
	}

	String method = p_args.get("method", "");
	if (method.is_empty()) {
		return mcp_error("method is required");
	}

	Dictionary err;
	Node *node = mcp_get_node(node_path, err);
	if (!node) {
		return err;
	}

	Array args_array = p_args.get("args", Array());

	// Build Variant argument pointers for callp.
	Vector<Variant> arg_variants;
	arg_variants.resize(args_array.size());
	for (int i = 0; i < args_array.size(); i++) {
		arg_variants.write[i] = args_array[i];
	}

	const Variant **argptrs = nullptr;
	Vector<const Variant *> argptr_vec;
	if (arg_variants.size() > 0) {
		argptr_vec.resize(arg_variants.size());
		for (int i = 0; i < arg_variants.size(); i++) {
			argptr_vec.write[i] = &arg_variants[i];
		}
		argptrs = const_cast<const Variant **>(argptr_vec.ptr());
	}

	Callable::CallError ce;
	Variant ret = node->callp(StringName(method), argptrs, arg_variants.size(), ce);

	if (ce.error != Callable::CallError::CALL_OK) {
		String error_msg;
		switch (ce.error) {
			case Callable::CallError::CALL_ERROR_INVALID_METHOD:
				error_msg = "Invalid method: " + method;
				break;
			case Callable::CallError::CALL_ERROR_INVALID_ARGUMENT:
				error_msg = "Invalid argument at index " + itos(ce.argument);
				break;
			case Callable::CallError::CALL_ERROR_TOO_MANY_ARGUMENTS:
				error_msg = "Too many arguments (expected " + itos(ce.expected) + ")";
				break;
			case Callable::CallError::CALL_ERROR_TOO_FEW_ARGUMENTS:
				error_msg = "Too few arguments (expected " + itos(ce.expected) + ")";
				break;
			default:
				error_msg = "Method call failed";
				break;
		}
		return mcp_error(error_msg);
	}

	Dictionary result;
	result["success"] = true;
	result["return_value"] = ret.stringify();
	result["return_type"] = Variant::get_type_name(ret.get_type());
	return result;
}

// --- set_runtime_property ---

static Dictionary mcp_tool_set_runtime_property(const Dictionary &p_args) {
	String node_path = p_args.get("node_path", "");
	if (node_path.is_empty()) {
		return mcp_error("node_path is required");
	}

	String property = p_args.get("property", "");
	if (property.is_empty()) {
		return mcp_error("property is required");
	}

	if (!p_args.has("value")) {
		return mcp_error("value is required");
	}

	Dictionary err;
	Node *node = mcp_get_node(node_path, err);
	if (!node) {
		return err;
	}

	Variant value = p_args["value"];
	bool valid = false;
	node->set(StringName(property), value, &valid);

	if (!valid) {
		return mcp_error("Failed to set property '" + property + "' on node");
	}

	Dictionary result;
	result["success"] = true;
	result["node_path"] = String(node->get_path());
	result["property"] = property;
	result["new_value"] = node->get(StringName(property)).stringify();
	return result;
}

// --- get_performance_metrics ---

static Dictionary mcp_tool_get_performance_metrics(const Dictionary &p_args) {
	Performance *perf = Performance::get_singleton();
	if (!perf) {
		return mcp_error("Performance singleton not available");
	}

	Dictionary result;
	result["success"] = true;

	Dictionary metrics;

	// Time metrics.
	metrics["fps"] = perf->get_monitor(Performance::TIME_FPS);
	metrics["process_time"] = perf->get_monitor(Performance::TIME_PROCESS);
	metrics["physics_process_time"] = perf->get_monitor(Performance::TIME_PHYSICS_PROCESS);
	metrics["navigation_process_time"] = perf->get_monitor(Performance::TIME_NAVIGATION_PROCESS);

	// Memory metrics.
	metrics["memory_static"] = perf->get_monitor(Performance::MEMORY_STATIC);
	metrics["memory_static_max"] = perf->get_monitor(Performance::MEMORY_STATIC_MAX);
	metrics["memory_message_buffer_max"] = perf->get_monitor(Performance::MEMORY_MESSAGE_BUFFER_MAX);

	// Object metrics.
	metrics["object_count"] = perf->get_monitor(Performance::OBJECT_COUNT);
	metrics["object_resource_count"] = perf->get_monitor(Performance::OBJECT_RESOURCE_COUNT);
	metrics["object_node_count"] = perf->get_monitor(Performance::OBJECT_NODE_COUNT);
	metrics["object_orphan_node_count"] = perf->get_monitor(Performance::OBJECT_ORPHAN_NODE_COUNT);

	// Render metrics.
	metrics["render_total_objects_in_frame"] = perf->get_monitor(Performance::RENDER_TOTAL_OBJECTS_IN_FRAME);
	metrics["render_total_primitives_in_frame"] = perf->get_monitor(Performance::RENDER_TOTAL_PRIMITIVES_IN_FRAME);
	metrics["render_total_draw_calls_in_frame"] = perf->get_monitor(Performance::RENDER_TOTAL_DRAW_CALLS_IN_FRAME);
	metrics["render_video_mem_used"] = perf->get_monitor(Performance::RENDER_VIDEO_MEM_USED);

	// Physics metrics.
	metrics["physics_2d_active_objects"] = perf->get_monitor(Performance::PHYSICS_2D_ACTIVE_OBJECTS);
	metrics["physics_2d_collision_pairs"] = perf->get_monitor(Performance::PHYSICS_2D_COLLISION_PAIRS);
	metrics["physics_2d_island_count"] = perf->get_monitor(Performance::PHYSICS_2D_ISLAND_COUNT);
	metrics["physics_3d_active_objects"] = perf->get_monitor(Performance::PHYSICS_3D_ACTIVE_OBJECTS);
	metrics["physics_3d_collision_pairs"] = perf->get_monitor(Performance::PHYSICS_3D_COLLISION_PAIRS);
	metrics["physics_3d_island_count"] = perf->get_monitor(Performance::PHYSICS_3D_ISLAND_COUNT);

	result["metrics"] = metrics;
	return result;
}

// --- get_physics_state_3d ---

static Dictionary mcp_tool_get_physics_state_3d(const Dictionary &p_args) {
	String node_path = p_args.get("node_path", "");
	if (node_path.is_empty()) {
		return mcp_error("node_path is required");
	}

	Dictionary err;
	Node *node = mcp_get_node(node_path, err);
	if (!node) {
		return err;
	}

	Dictionary result;
	result["success"] = true;
	result["node_path"] = String(node->get_path());
	result["class"] = node->get_class();

	Dictionary state;

	// Try PhysicsBody3D (base class for RigidBody3D, CharacterBody3D, etc.).
	PhysicsBody3D *body = Object::cast_to<PhysicsBody3D>(node);
	if (!body) {
		return mcp_error("Node is not a PhysicsBody3D: " + node_path);
	}

	Node3D *n3d = Object::cast_to<Node3D>(node);
	if (n3d) {
		Vector3 pos = n3d->get_global_position();
		state["global_position"] = String("(%s, %s, %s)").replace("%s", String::num(pos.x)).replace("%s", String::num(pos.y)).replace("%s", String::num(pos.z));
		Vector3 rot = n3d->get_global_rotation_degrees();
		state["global_rotation_degrees"] = String("(%s, %s, %s)").replace("%s", String::num(rot.x)).replace("%s", String::num(rot.y)).replace("%s", String::num(rot.z));
	}

	// RigidBody3D specific.
	RigidBody3D *rigid = Object::cast_to<RigidBody3D>(node);
	if (rigid) {
		Vector3 lv = rigid->get_linear_velocity();
		state["linear_velocity"] = vformat("(%s, %s, %s)", String::num(lv.x), String::num(lv.y), String::num(lv.z));
		Vector3 av = rigid->get_angular_velocity();
		state["angular_velocity"] = vformat("(%s, %s, %s)", String::num(av.x), String::num(av.y), String::num(av.z));
		state["mass"] = rigid->get_mass();
		state["gravity_scale"] = rigid->get_gravity_scale();
		state["sleeping"] = rigid->is_sleeping();
		state["freeze"] = rigid->is_freeze_enabled();

		// Contact info.
		state["contact_count"] = rigid->get_contact_count();
	}

	// CharacterBody3D specific.
	CharacterBody3D *character = Object::cast_to<CharacterBody3D>(node);
	if (character) {
		Vector3 vel = character->get_velocity();
		state["velocity"] = vformat("(%s, %s, %s)", String::num(vel.x), String::num(vel.y), String::num(vel.z));
		state["is_on_floor"] = character->is_on_floor();
		state["is_on_wall"] = character->is_on_wall();
		state["is_on_ceiling"] = character->is_on_ceiling();
	}

	result["physics_state"] = state;
	return result;
}

// --- Registration ---

void mcp_register_runtime_tools(MCPServer *p_server) {
	// get_node_runtime_state
	{
		Dictionary schema = MCPSchema::object()
									.prop("node_path", "string", "Path to the node in the scene tree", true)
									.build();
		p_server->register_tool("get_node_runtime_state",
				"Get all properties of a node at runtime by iterating its property list",
				schema, callable_mp_static(&mcp_tool_get_node_runtime_state));
	}

	// call_node_method
	{
		Dictionary schema = MCPSchema::object()
									.prop("node_path", "string", "Path to the node in the scene tree", true)
									.prop("method", "string", "Method name to call on the node", true)
									.prop_array("args", "Array of arguments to pass to the method")
									.build();
		p_server->register_tool("call_node_method",
				"Call a method on a node and return the result",
				schema, callable_mp_static(&mcp_tool_call_node_method));
	}

	// set_runtime_property
	{
		Dictionary schema = MCPSchema::object()
									.prop("node_path", "string", "Path to the node in the scene tree", true)
									.prop("property", "string", "Property name to set", true)
									.prop("value", "string", "Value to set the property to", true)
									.build();
		p_server->register_tool("set_runtime_property",
				"Set a property on a running node at runtime",
				schema, callable_mp_static(&mcp_tool_set_runtime_property));
	}

	// get_performance_metrics
	{
		Dictionary schema = MCPSchema::object().build();
		p_server->register_tool("get_performance_metrics",
				"Get engine performance metrics including FPS, draw calls, memory usage, physics stats",
				schema, callable_mp_static(&mcp_tool_get_performance_metrics));
	}

	// get_physics_state_3d
	{
		Dictionary schema = MCPSchema::object()
									.prop("node_path", "string", "Path to a PhysicsBody3D node", true)
									.build();
		p_server->register_tool("get_physics_state_3d",
				"Get physics body velocity, position, contacts, and other physics state for a 3D physics node",
				schema, callable_mp_static(&mcp_tool_get_physics_state_3d));
	}
}
