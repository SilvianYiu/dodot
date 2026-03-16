/**************************************************************************/
/*  mcp_tools_navigation.cpp                                              */
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

#include "mcp_tools_navigation.h"

#include "../mcp_server.h"
#include "../mcp_tool_helpers.h"

#include "scene/3d/navigation/navigation_agent_3d.h"
#include "scene/3d/navigation/navigation_region_3d.h"
#include "scene/resources/navigation_mesh.h"

// --- Tool handlers ---

static Dictionary mcp_tool_create_navigation_region(const Dictionary &p_args) {
	String parent_path = p_args.get("parent_path", "/root");
	String name = p_args.get("name", "NavigationRegion3D");

	Dictionary err;
	Node *parent = mcp_get_or_create_parent(parent_path, err);
	if (!parent) {
		return err;
	}

	NavigationRegion3D *region = memnew(NavigationRegion3D);
	region->set_name(name);

	// Create and assign a default NavigationMesh.
	Ref<NavigationMesh> nav_mesh;
	nav_mesh.instantiate();
	region->set_navigation_mesh(nav_mesh);

	mcp_add_child_to_scene(parent, region);

	Dictionary result = mcp_success();
	result["node"] = mcp_get_node_info(region);
	return result;
}

static Dictionary mcp_tool_create_navigation_agent(const Dictionary &p_args) {
	String parent_path = p_args.get("parent_path", "/root");
	String name = p_args.get("name", "NavigationAgent3D");
	Dictionary properties = p_args.get("properties", Dictionary());

	Dictionary err;
	Node *parent = mcp_get_or_create_parent(parent_path, err);
	if (!parent) {
		return err;
	}

	NavigationAgent3D *agent = memnew(NavigationAgent3D);
	agent->set_name(name);

	if (properties.has("path_desired_distance")) {
		agent->set_path_desired_distance(properties["path_desired_distance"]);
	}
	if (properties.has("target_desired_distance")) {
		agent->set_target_desired_distance(properties["target_desired_distance"]);
	}
	if (properties.has("radius")) {
		agent->set_radius(properties["radius"]);
	}
	if (properties.has("max_speed")) {
		agent->set_max_speed(properties["max_speed"]);
	}
	if (properties.has("avoidance_enabled")) {
		agent->set_avoidance_enabled(properties["avoidance_enabled"]);
	}

	mcp_add_child_to_scene(parent, agent);

	Dictionary result = mcp_success();
	result["node"] = mcp_get_node_info(agent);
	return result;
}

static Dictionary mcp_tool_bake_navigation_mesh(const Dictionary &p_args) {
	String region_path = p_args.get("region_path", "");

	Dictionary err;
	Node *node = mcp_get_node(region_path, err);
	if (!node) {
		return err;
	}

	NavigationRegion3D *region = Object::cast_to<NavigationRegion3D>(node);
	if (!region) {
		return mcp_error("Node is not a NavigationRegion3D: " + region_path);
	}

	Ref<NavigationMesh> nav_mesh = region->get_navigation_mesh();
	if (nav_mesh.is_null()) {
		return mcp_error("NavigationRegion3D has no NavigationMesh assigned: " + region_path);
	}

	// Bake on the main thread (p_on_thread = false).
	region->bake_navigation_mesh(false);

	Dictionary result = mcp_success();
	result["message"] = "Navigation mesh bake initiated for: " + region_path;
	return result;
}

static Dictionary mcp_tool_set_navigation_target(const Dictionary &p_args) {
	String agent_path = p_args.get("agent_path", "");
	Dictionary target_pos = p_args.get("target_position", Dictionary());

	Dictionary err;
	Node *node = mcp_get_node(agent_path, err);
	if (!node) {
		return err;
	}

	NavigationAgent3D *agent = Object::cast_to<NavigationAgent3D>(node);
	if (!agent) {
		return mcp_error("Node is not a NavigationAgent3D: " + agent_path);
	}

	Vector3 target(
			target_pos.get("x", 0.0),
			target_pos.get("y", 0.0),
			target_pos.get("z", 0.0));

	agent->set_target_position(target);

	Dictionary result = mcp_success();
	result["target_x"] = target.x;
	result["target_y"] = target.y;
	result["target_z"] = target.z;
	return result;
}

static Dictionary mcp_tool_get_navigation_path(const Dictionary &p_args) {
	String agent_path = p_args.get("agent_path", "");

	Dictionary err;
	Node *node = mcp_get_node(agent_path, err);
	if (!node) {
		return err;
	}

	NavigationAgent3D *agent = Object::cast_to<NavigationAgent3D>(node);
	if (!agent) {
		return mcp_error("Node is not a NavigationAgent3D: " + agent_path);
	}

	const Vector<Vector3> &path = agent->get_current_navigation_path();
	Array path_array;
	for (int i = 0; i < path.size(); i++) {
		Dictionary point;
		point["x"] = path[i].x;
		point["y"] = path[i].y;
		point["z"] = path[i].z;
		path_array.push_back(point);
	}

	Dictionary result = mcp_success();
	result["path"] = path_array;
	result["path_index"] = agent->get_current_navigation_path_index();
	result["is_navigation_finished"] = agent->is_navigation_finished();

	Vector3 next_pos = agent->get_next_path_position();
	Dictionary next;
	next["x"] = next_pos.x;
	next["y"] = next_pos.y;
	next["z"] = next_pos.z;
	result["next_position"] = next;

	return result;
}

// --- Registration ---

void mcp_register_navigation_tools(MCPServer *p_server) {
	// create_navigation_region
	{
		Dictionary schema = MCPSchema::object()
									.prop("parent_path", "string", "Parent node path", true)
									.prop("name", "string", "Node name")
									.build();

		p_server->register_tool("create_navigation_region",
				"Create a NavigationRegion3D with a default NavigationMesh",
				schema, callable_mp_static(&mcp_tool_create_navigation_region));
	}

	// create_navigation_agent
	{
		Dictionary schema = MCPSchema::object()
									.prop("parent_path", "string", "Parent node path (should be a CharacterBody3D or similar)", true)
									.prop("name", "string", "Node name")
									.prop_object("properties", "Agent properties: path_desired_distance, target_desired_distance, radius, max_speed, avoidance_enabled")
									.build();

		p_server->register_tool("create_navigation_agent",
				"Create a NavigationAgent3D node",
				schema, callable_mp_static(&mcp_tool_create_navigation_agent));
	}

	// bake_navigation_mesh
	{
		Dictionary schema = MCPSchema::object()
									.prop("region_path", "string", "Path to the NavigationRegion3D node", true)
									.build();

		p_server->register_tool("bake_navigation_mesh",
				"Bake the navigation mesh for a NavigationRegion3D",
				schema, callable_mp_static(&mcp_tool_bake_navigation_mesh));
	}

	// set_navigation_target
	{
		Dictionary schema = MCPSchema::object()
									.prop("agent_path", "string", "Path to the NavigationAgent3D node", true)
									.prop_object("target_position", "Target position {x, y, z}", true)
									.build();

		p_server->register_tool("set_navigation_target",
				"Set the navigation target position for a NavigationAgent3D",
				schema, callable_mp_static(&mcp_tool_set_navigation_target));
	}

	// get_navigation_path
	{
		Dictionary schema = MCPSchema::object()
									.prop("agent_path", "string", "Path to the NavigationAgent3D node", true)
									.build();

		p_server->register_tool("get_navigation_path",
				"Get the computed navigation path from a NavigationAgent3D",
				schema, callable_mp_static(&mcp_tool_get_navigation_path));
	}
}
