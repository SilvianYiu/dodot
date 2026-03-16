/**************************************************************************/
/*  mcp_tools_3d_physics.cpp                                              */
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

#include "mcp_tools_3d_physics.h"

#include "../mcp_server.h"
#include "../mcp_tool_helpers.h"

#include "scene/3d/mesh_instance_3d.h"
#include "scene/main/viewport.h"
#include "scene/main/window.h"
#include "scene/3d/physics/area_3d.h"
#include "scene/3d/physics/character_body_3d.h"
#include "scene/3d/physics/collision_shape_3d.h"
#include "scene/3d/physics/rigid_body_3d.h"
#include "scene/3d/physics/static_body_3d.h"
#include "scene/resources/3d/box_shape_3d.h"
#include "scene/resources/3d/capsule_shape_3d.h"
#include "scene/resources/3d/cylinder_shape_3d.h"
#include "scene/resources/3d/shape_3d.h"
#include "scene/resources/3d/sphere_shape_3d.h"
#include "scene/resources/3d/world_3d.h"
#include "servers/physics_3d/physics_server_3d.h"

// --- Shape creation helper ---

static Ref<Shape3D> _create_shape(const String &p_type, const Dictionary &p_params) {
	if (p_type == "box") {
		Ref<BoxShape3D> s;
		s.instantiate();
		s->set_size(Vector3(
				p_params.get("size_x", 1.0),
				p_params.get("size_y", 1.0),
				p_params.get("size_z", 1.0)));
		return s;
	} else if (p_type == "sphere") {
		Ref<SphereShape3D> s;
		s.instantiate();
		s->set_radius(p_params.get("radius", 0.5));
		return s;
	} else if (p_type == "capsule") {
		Ref<CapsuleShape3D> s;
		s.instantiate();
		s->set_radius(p_params.get("radius", 0.5));
		s->set_height(p_params.get("height", 2.0));
		return s;
	} else if (p_type == "cylinder") {
		Ref<CylinderShape3D> s;
		s.instantiate();
		s->set_radius(p_params.get("radius", 0.5));
		s->set_height(p_params.get("height", 2.0));
		return s;
	}
	return Ref<Shape3D>();
}

static CollisionShape3D *_create_collision_shape_node(const String &p_shape_type, const Dictionary &p_shape_params, Dictionary &r_error) {
	Ref<Shape3D> shape = _create_shape(p_shape_type, p_shape_params);
	if (shape.is_null()) {
		r_error = mcp_error("Unknown shape type: " + p_shape_type + ". Use box, sphere, capsule, or cylinder.");
		return nullptr;
	}
	CollisionShape3D *col = memnew(CollisionShape3D);
	col->set_name("CollisionShape3D");
	col->set_shape(shape);
	return col;
}

// --- Tool handlers ---

static Dictionary mcp_tool_create_static_body(const Dictionary &p_args) {
	String parent_path = p_args.get("parent_path", "/root");
	String name = p_args.get("name", "StaticBody3D");
	String shape_type = p_args.get("shape_type", "box");
	Dictionary shape_params = p_args.get("shape_params", Dictionary());

	Dictionary err;
	Node *parent = mcp_get_or_create_parent(parent_path, err);
	if (!parent) {
		return err;
	}

	StaticBody3D *body = memnew(StaticBody3D);
	body->set_name(name);
	mcp_add_child_to_scene(parent, body);

	CollisionShape3D *col = _create_collision_shape_node(shape_type, shape_params, err);
	if (!col) {
		return err;
	}
	mcp_add_child_to_scene(body, col);

	Dictionary result = mcp_success();
	result["node"] = mcp_get_node_info(body);
	return result;
}

static Dictionary mcp_tool_create_rigid_body(const Dictionary &p_args) {
	String parent_path = p_args.get("parent_path", "/root");
	String name = p_args.get("name", "RigidBody3D");
	String shape_type = p_args.get("shape_type", "box");
	Dictionary shape_params = p_args.get("shape_params", Dictionary());
	Dictionary properties = p_args.get("properties", Dictionary());

	Dictionary err;
	Node *parent = mcp_get_or_create_parent(parent_path, err);
	if (!parent) {
		return err;
	}

	RigidBody3D *body = memnew(RigidBody3D);
	body->set_name(name);

	if (properties.has("mass")) {
		body->set_mass(properties["mass"]);
	}
	if (properties.has("gravity_scale")) {
		body->set_gravity_scale(properties["gravity_scale"]);
	}
	if (properties.has("linear_damp")) {
		body->set_linear_damp(properties["linear_damp"]);
	}
	if (properties.has("angular_damp")) {
		body->set_angular_damp(properties["angular_damp"]);
	}

	mcp_add_child_to_scene(parent, body);

	CollisionShape3D *col = _create_collision_shape_node(shape_type, shape_params, err);
	if (!col) {
		return err;
	}
	mcp_add_child_to_scene(body, col);

	// Add optional MeshInstance3D for visibility.
	bool add_mesh = properties.get("add_mesh", false);
	if (add_mesh) {
		MeshInstance3D *mesh_inst = memnew(MeshInstance3D);
		mesh_inst->set_name("MeshInstance3D");
		mcp_add_child_to_scene(body, mesh_inst);
	}

	Dictionary result = mcp_success();
	result["node"] = mcp_get_node_info(body);
	return result;
}

static Dictionary mcp_tool_create_character_body_3d(const Dictionary &p_args) {
	String parent_path = p_args.get("parent_path", "/root");
	String name = p_args.get("name", "CharacterBody3D");
	String shape_type = p_args.get("shape_type", "capsule");
	Dictionary shape_params = p_args.get("shape_params", Dictionary());

	Dictionary err;
	Node *parent = mcp_get_or_create_parent(parent_path, err);
	if (!parent) {
		return err;
	}

	CharacterBody3D *body = memnew(CharacterBody3D);
	body->set_name(name);
	mcp_add_child_to_scene(parent, body);

	CollisionShape3D *col = _create_collision_shape_node(shape_type, shape_params, err);
	if (!col) {
		return err;
	}
	mcp_add_child_to_scene(body, col);

	Dictionary result = mcp_success();
	result["node"] = mcp_get_node_info(body);
	return result;
}

static Dictionary mcp_tool_create_collision_shape(const Dictionary &p_args) {
	String parent_path = p_args.get("parent_path", "/root");
	String name = p_args.get("name", "CollisionShape3D");
	String shape_type = p_args.get("shape_type", "box");
	Dictionary params = p_args.get("params", Dictionary());

	Dictionary err;
	Node *parent = mcp_get_or_create_parent(parent_path, err);
	if (!parent) {
		return err;
	}

	CollisionShape3D *col = _create_collision_shape_node(shape_type, params, err);
	if (!col) {
		return err;
	}
	col->set_name(name);
	mcp_add_child_to_scene(parent, col);

	Dictionary result = mcp_success();
	result["node"] = mcp_get_node_info(col);
	return result;
}

static Dictionary mcp_tool_create_area_3d(const Dictionary &p_args) {
	String parent_path = p_args.get("parent_path", "/root");
	String name = p_args.get("name", "Area3D");
	String shape_type = p_args.get("shape_type", "box");
	Dictionary shape_params = p_args.get("shape_params", Dictionary());

	Dictionary err;
	Node *parent = mcp_get_or_create_parent(parent_path, err);
	if (!parent) {
		return err;
	}

	Area3D *area = memnew(Area3D);
	area->set_name(name);
	mcp_add_child_to_scene(parent, area);

	CollisionShape3D *col = _create_collision_shape_node(shape_type, shape_params, err);
	if (!col) {
		return err;
	}
	mcp_add_child_to_scene(area, col);

	Dictionary result = mcp_success();
	result["node"] = mcp_get_node_info(area);
	return result;
}

static Dictionary mcp_tool_raycast_query(const Dictionary &p_args) {
	Dictionary from_dict = p_args.get("from", Dictionary());
	Dictionary to_dict = p_args.get("to", Dictionary());
	uint32_t collision_mask = p_args.get("collision_mask", UINT32_MAX);

	Vector3 from(
			from_dict.get("x", 0.0),
			from_dict.get("y", 0.0),
			from_dict.get("z", 0.0));
	Vector3 to(
			to_dict.get("x", 0.0),
			to_dict.get("y", 0.0),
			to_dict.get("z", 0.0));

	SceneTree *st = mcp_get_scene_tree();
	if (!st) {
		return mcp_error("No SceneTree available");
	}

	Viewport *viewport = st->get_root();
	if (!viewport) {
		return mcp_error("No root viewport");
	}

	Ref<World3D> world = viewport->get_world_3d();
	if (world.is_null()) {
		return mcp_error("No World3D available");
	}

	PhysicsDirectSpaceState3D *space_state = world->get_direct_space_state();
	if (!space_state) {
		return mcp_error("No physics space state available");
	}

	PhysicsDirectSpaceState3D::RayParameters ray_params;
	ray_params.from = from;
	ray_params.to = to;
	ray_params.collision_mask = collision_mask;

	PhysicsDirectSpaceState3D::RayResult ray_result;
	bool hit = space_state->intersect_ray(ray_params, ray_result);

	Dictionary result = mcp_success();
	result["hit"] = hit;
	if (hit) {
		Dictionary hit_info;
		hit_info["position_x"] = ray_result.position.x;
		hit_info["position_y"] = ray_result.position.y;
		hit_info["position_z"] = ray_result.position.z;
		hit_info["normal_x"] = ray_result.normal.x;
		hit_info["normal_y"] = ray_result.normal.y;
		hit_info["normal_z"] = ray_result.normal.z;
		hit_info["shape"] = ray_result.shape;
		if (ray_result.collider) {
			Node *collider_node = Object::cast_to<Node>(ray_result.collider);
			if (collider_node) {
				hit_info["collider_path"] = String(collider_node->get_path());
				hit_info["collider_name"] = collider_node->get_name();
			}
		}
		result["hit_info"] = hit_info;
	}
	return result;
}

static Dictionary mcp_tool_get_physics_state(const Dictionary &p_args) {
	String node_path = p_args.get("node_path", "");

	Dictionary err;
	Node *node = mcp_get_node(node_path, err);
	if (!node) {
		return err;
	}

	Dictionary result = mcp_success();
	Dictionary state;

	RigidBody3D *rigid = Object::cast_to<RigidBody3D>(node);
	if (rigid) {
		state["type"] = "RigidBody3D";
		state["mass"] = rigid->get_mass();
		state["gravity_scale"] = rigid->get_gravity_scale();
		Vector3 lv = rigid->get_linear_velocity();
		state["linear_velocity_x"] = lv.x;
		state["linear_velocity_y"] = lv.y;
		state["linear_velocity_z"] = lv.z;
		Vector3 av = rigid->get_angular_velocity();
		state["angular_velocity_x"] = av.x;
		state["angular_velocity_y"] = av.y;
		state["angular_velocity_z"] = av.z;
		state["sleeping"] = rigid->is_sleeping();
		state["freeze"] = rigid->is_freeze_enabled();
		result["state"] = state;
		return result;
	}

	CharacterBody3D *character = Object::cast_to<CharacterBody3D>(node);
	if (character) {
		state["type"] = "CharacterBody3D";
		Vector3 vel = character->get_velocity();
		state["velocity_x"] = vel.x;
		state["velocity_y"] = vel.y;
		state["velocity_z"] = vel.z;
		state["on_floor"] = character->is_on_floor();
		state["on_wall"] = character->is_on_wall();
		state["on_ceiling"] = character->is_on_ceiling();
		result["state"] = state;
		return result;
	}

	StaticBody3D *static_body = Object::cast_to<StaticBody3D>(node);
	if (static_body) {
		state["type"] = "StaticBody3D";
		result["state"] = state;
		return result;
	}

	Area3D *area = Object::cast_to<Area3D>(node);
	if (area) {
		state["type"] = "Area3D";
		state["monitoring"] = area->is_monitoring();
		state["monitorable"] = area->is_monitorable();
		state["gravity"] = area->get_gravity();
		TypedArray<Node3D> overlapping = area->get_overlapping_bodies();
		Array body_paths;
		for (int i = 0; i < overlapping.size(); i++) {
			Node *n = Object::cast_to<Node>(overlapping[i]);
			if (n) {
				body_paths.push_back(String(n->get_path()));
			}
		}
		state["overlapping_bodies"] = body_paths;
		result["state"] = state;
		return result;
	}

	return mcp_error("Node is not a physics body: " + node_path);
}

// --- Registration ---

void mcp_register_3d_physics_tools(MCPServer *p_server) {
	// create_static_body
	{
		Array shape_types;
		shape_types.push_back("box");
		shape_types.push_back("sphere");
		shape_types.push_back("capsule");
		shape_types.push_back("cylinder");

		Dictionary schema = MCPSchema::object()
									.prop("parent_path", "string", "Parent node path", true)
									.prop("name", "string", "Node name", true)
									.prop_enum("shape_type", shape_types, "Collision shape type", true)
									.prop_object("shape_params", "Shape parameters (size_x/y/z for box, radius for sphere, radius+height for capsule/cylinder)")
									.build();

		p_server->register_tool("create_static_body",
				"Create a StaticBody3D with a CollisionShape3D child",
				schema, callable_mp_static(&mcp_tool_create_static_body));
	}

	// create_rigid_body
	{
		Array shape_types;
		shape_types.push_back("box");
		shape_types.push_back("sphere");
		shape_types.push_back("capsule");
		shape_types.push_back("cylinder");

		Dictionary schema = MCPSchema::object()
									.prop("parent_path", "string", "Parent node path", true)
									.prop("name", "string", "Node name", true)
									.prop_enum("shape_type", shape_types, "Collision shape type", true)
									.prop_object("shape_params", "Shape parameters (size_x/y/z for box, radius for sphere, radius+height for capsule/cylinder)")
									.prop_object("properties", "Body properties: mass, gravity_scale, linear_damp, angular_damp, add_mesh(bool)")
									.build();

		p_server->register_tool("create_rigid_body",
				"Create a RigidBody3D with CollisionShape3D and optional MeshInstance3D",
				schema, callable_mp_static(&mcp_tool_create_rigid_body));
	}

	// create_character_body_3d
	{
		Array shape_types;
		shape_types.push_back("box");
		shape_types.push_back("sphere");
		shape_types.push_back("capsule");
		shape_types.push_back("cylinder");

		Dictionary schema = MCPSchema::object()
									.prop("parent_path", "string", "Parent node path", true)
									.prop("name", "string", "Node name", true)
									.prop_enum("shape_type", shape_types, "Collision shape type", true)
									.prop_object("shape_params", "Shape parameters (size_x/y/z for box, radius for sphere, radius+height for capsule/cylinder)")
									.build();

		p_server->register_tool("create_character_body_3d",
				"Create a CharacterBody3D with a CollisionShape3D child",
				schema, callable_mp_static(&mcp_tool_create_character_body_3d));
	}

	// create_collision_shape
	{
		Array shape_types;
		shape_types.push_back("box");
		shape_types.push_back("sphere");
		shape_types.push_back("capsule");
		shape_types.push_back("cylinder");

		Dictionary schema = MCPSchema::object()
									.prop("parent_path", "string", "Parent node path", true)
									.prop("name", "string", "Node name")
									.prop_enum("shape_type", shape_types, "Shape type", true)
									.prop_object("params", "Shape parameters (size_x/y/z for box, radius for sphere, radius+height for capsule/cylinder)")
									.build();

		p_server->register_tool("create_collision_shape",
				"Create a standalone CollisionShape3D node",
				schema, callable_mp_static(&mcp_tool_create_collision_shape));
	}

	// create_area_3d
	{
		Array shape_types;
		shape_types.push_back("box");
		shape_types.push_back("sphere");
		shape_types.push_back("capsule");
		shape_types.push_back("cylinder");

		Dictionary schema = MCPSchema::object()
									.prop("parent_path", "string", "Parent node path", true)
									.prop("name", "string", "Node name", true)
									.prop_enum("shape_type", shape_types, "Collision shape type", true)
									.prop_object("shape_params", "Shape parameters (size_x/y/z for box, radius for sphere, radius+height for capsule/cylinder)")
									.build();

		p_server->register_tool("create_area_3d",
				"Create an Area3D with a CollisionShape3D child",
				schema, callable_mp_static(&mcp_tool_create_area_3d));
	}

	// raycast_query
	{
		Dictionary schema = MCPSchema::object()
									.prop_object("from", "Ray origin {x, y, z}", true)
									.prop_object("to", "Ray destination {x, y, z}", true)
									.prop_number("collision_mask", "Collision mask (default: all layers)")
									.build();

		p_server->register_tool("raycast_query",
				"Perform a physics raycast and return hit information",
				schema, callable_mp_static(&mcp_tool_raycast_query));
	}

	// get_physics_state
	{
		Dictionary schema = MCPSchema::object()
									.prop("node_path", "string", "Path to physics body node", true)
									.build();

		p_server->register_tool("get_physics_state",
				"Get physics body state (velocity, contacts, etc.)",
				schema, callable_mp_static(&mcp_tool_get_physics_state));
	}
}
