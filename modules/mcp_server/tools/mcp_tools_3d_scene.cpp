/**************************************************************************/
/*  mcp_tools_3d_scene.cpp                                                */
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

#include "mcp_tools_3d_scene.h"

#include "../mcp_server.h"
#include "../mcp_tool_helpers.h"

#include "scene/3d/camera_3d.h"
#include "scene/3d/light_3d.h"
#include "scene/3d/mesh_instance_3d.h"
#include "scene/3d/world_environment.h"
#include "scene/resources/3d/primitive_meshes.h"
#include "scene/resources/3d/sky_material.h"
#include "scene/resources/environment.h"
#include "scene/resources/sky.h"

// ---------------------------------------------------------------------------
// create_mesh_instance
// ---------------------------------------------------------------------------

static Dictionary _tool_create_mesh_instance(const Dictionary &p_args) {
	String parent_path = p_args.get("parent_path", "/root");
	String name = p_args.get("name", "MeshInstance3D");
	String mesh_type = p_args.get("mesh_type", "BoxMesh");
	Dictionary properties = p_args.get("properties", Dictionary());

	Dictionary err;
	Node *parent = mcp_get_or_create_parent(parent_path, err);
	if (!parent) {
		return err;
	}

	MeshInstance3D *mesh_instance = memnew(MeshInstance3D);
	mesh_instance->set_name(name);

	Ref<Mesh> mesh;
	if (mesh_type == "BoxMesh") {
		mesh = Ref<BoxMesh>(memnew(BoxMesh));
	} else if (mesh_type == "SphereMesh") {
		mesh = Ref<SphereMesh>(memnew(SphereMesh));
	} else if (mesh_type == "CylinderMesh") {
		mesh = Ref<CylinderMesh>(memnew(CylinderMesh));
	} else if (mesh_type == "PlaneMesh") {
		mesh = Ref<PlaneMesh>(memnew(PlaneMesh));
	} else if (mesh_type == "CapsuleMesh") {
		mesh = Ref<CapsuleMesh>(memnew(CapsuleMesh));
	} else if (mesh_type == "PrismMesh") {
		mesh = Ref<PrismMesh>(memnew(PrismMesh));
	} else if (mesh_type == "TorusMesh") {
		mesh = Ref<TorusMesh>(memnew(TorusMesh));
	} else {
		memdelete(mesh_instance);
		return mcp_error("Unknown mesh type: " + mesh_type + ". Supported: BoxMesh, SphereMesh, CylinderMesh, PlaneMesh, CapsuleMesh, PrismMesh, TorusMesh");
	}

	// Apply mesh-level properties (e.g., size, radius, height).
	for (const Variant *key = properties.next(nullptr); key; key = properties.next(key)) {
		String prop_name = *key;
		List<PropertyInfo> prop_list;
		mesh->get_property_list(&prop_list);
		if (mesh->has_method("set_" + prop_name) || prop_list.size() > 0) {
			mesh->set(StringName(prop_name), properties[*key]);
		}
	}

	mesh_instance->set_mesh(mesh);
	mcp_add_child_to_scene(parent, mesh_instance);

	Dictionary result = mcp_success();
	result["node_path"] = String(mesh_instance->get_path());
	result["mesh_type"] = mesh_type;
	return result;
}

// ---------------------------------------------------------------------------
// create_camera_3d
// ---------------------------------------------------------------------------

static Dictionary _tool_create_camera_3d(const Dictionary &p_args) {
	String parent_path = p_args.get("parent_path", "/root");
	String name = p_args.get("name", "Camera3D");
	Dictionary properties = p_args.get("properties", Dictionary());

	Dictionary err;
	Node *parent = mcp_get_or_create_parent(parent_path, err);
	if (!parent) {
		return err;
	}

	Camera3D *camera = memnew(Camera3D);
	camera->set_name(name);

	mcp_set_properties(camera, properties);
	mcp_add_child_to_scene(parent, camera);

	Dictionary result = mcp_success();
	result["node_path"] = String(camera->get_path());
	return result;
}

// ---------------------------------------------------------------------------
// create_light_3d
// ---------------------------------------------------------------------------

static Dictionary _tool_create_light_3d(const Dictionary &p_args) {
	String parent_path = p_args.get("parent_path", "/root");
	String name = p_args.get("name", "Light3D");
	String light_type = p_args.get("light_type", "DirectionalLight3D");
	Dictionary properties = p_args.get("properties", Dictionary());

	Dictionary err;
	Node *parent = mcp_get_or_create_parent(parent_path, err);
	if (!parent) {
		return err;
	}

	Light3D *light = nullptr;
	if (light_type == "DirectionalLight3D") {
		light = memnew(DirectionalLight3D);
	} else if (light_type == "OmniLight3D") {
		light = memnew(OmniLight3D);
	} else if (light_type == "SpotLight3D") {
		light = memnew(SpotLight3D);
	} else {
		return mcp_error("Unknown light type: " + light_type + ". Supported: DirectionalLight3D, OmniLight3D, SpotLight3D");
	}

	light->set_name(name);
	mcp_set_properties(light, properties);
	mcp_add_child_to_scene(parent, light);

	Dictionary result = mcp_success();
	result["node_path"] = String(light->get_path());
	result["light_type"] = light_type;
	return result;
}

// ---------------------------------------------------------------------------
// create_environment
// ---------------------------------------------------------------------------

static Dictionary _tool_create_environment(const Dictionary &p_args) {
	String parent_path = p_args.get("parent_path", "/root");
	String name = p_args.get("name", "WorldEnvironment");
	String sky_type = p_args.get("sky_type", "procedural");

	Dictionary err;
	Node *parent = mcp_get_or_create_parent(parent_path, err);
	if (!parent) {
		return err;
	}

	WorldEnvironment *world_env = memnew(WorldEnvironment);
	world_env->set_name(name);

	Ref<Environment> env;
	env.instantiate();

	if (sky_type == "procedural") {
		Ref<Sky> sky;
		sky.instantiate();

		Ref<ProceduralSkyMaterial> sky_mat;
		sky_mat.instantiate();

		sky->set_material(sky_mat);
		env->set_sky(sky);
		env->set_background(Environment::BG_SKY);
	} else if (sky_type == "clear_color") {
		env->set_background(Environment::BG_CLEAR_COLOR);
	} else if (sky_type == "color") {
		env->set_background(Environment::BG_COLOR);
	} else {
		memdelete(world_env);
		return mcp_error("Unknown sky_type: " + sky_type + ". Supported: procedural, clear_color, color");
	}

	world_env->set_environment(env);
	mcp_add_child_to_scene(parent, world_env);

	Dictionary result = mcp_success();
	result["node_path"] = String(world_env->get_path());
	result["sky_type"] = sky_type;
	return result;
}

// ---------------------------------------------------------------------------
// create_csg_shape
// ---------------------------------------------------------------------------

static Dictionary _tool_create_csg_shape(const Dictionary &p_args) {
	String parent_path = p_args.get("parent_path", "/root");
	String name = p_args.get("name", "CSGShape3D");
	String shape_type = p_args.get("shape_type", "CSGBox3D");
	Dictionary properties = p_args.get("properties", Dictionary());

	// Validate shape type.
	if (shape_type != "CSGBox3D" && shape_type != "CSGSphere3D" &&
			shape_type != "CSGCylinder3D" && shape_type != "CSGTorus3D" &&
			shape_type != "CSGPolygon3D" && shape_type != "CSGMesh3D" &&
			shape_type != "CSGCombiner3D") {
		return mcp_error("Unknown CSG shape type: " + shape_type + ". Supported: CSGBox3D, CSGSphere3D, CSGCylinder3D, CSGTorus3D, CSGPolygon3D, CSGMesh3D, CSGCombiner3D");
	}

	Dictionary err;
	Node *parent = mcp_get_or_create_parent(parent_path, err);
	if (!parent) {
		return err;
	}

	Node *csg_node = mcp_create_node_of_type(shape_type, name, err);
	if (!csg_node) {
		return err;
	}

	mcp_set_properties(csg_node, properties);
	mcp_add_child_to_scene(parent, csg_node);

	Dictionary result = mcp_success();
	result["node_path"] = String(csg_node->get_path());
	result["shape_type"] = shape_type;
	return result;
}

// ---------------------------------------------------------------------------
// Registration
// ---------------------------------------------------------------------------

void mcp_register_3d_scene_tools(MCPServer *p_server) {
	// create_mesh_instance
	{
		Array mesh_types;
		mesh_types.push_back("BoxMesh");
		mesh_types.push_back("SphereMesh");
		mesh_types.push_back("CylinderMesh");
		mesh_types.push_back("PlaneMesh");
		mesh_types.push_back("CapsuleMesh");
		mesh_types.push_back("PrismMesh");
		mesh_types.push_back("TorusMesh");

		Dictionary schema = MCPSchema::object()
									.prop("parent_path", "string", "Path to the parent node (default: /root)", true)
									.prop("name", "string", "Name for the MeshInstance3D node", true)
									.prop_enum("mesh_type", mesh_types, "Type of primitive mesh to create", true)
									.prop_object("properties", "Mesh properties (e.g., size, radius, height)")
									.build();

		p_server->register_tool("create_mesh_instance",
				"Create a MeshInstance3D with a primitive mesh (BoxMesh, SphereMesh, CylinderMesh, PlaneMesh, CapsuleMesh, PrismMesh, TorusMesh)",
				schema, callable_mp_static(_tool_create_mesh_instance));
	}

	// create_camera_3d
	{
		Dictionary schema = MCPSchema::object()
									.prop("parent_path", "string", "Path to the parent node (default: /root)", true)
									.prop("name", "string", "Name for the Camera3D node", true)
									.prop_object("properties", "Camera properties (e.g., fov, near, far)")
									.build();

		p_server->register_tool("create_camera_3d",
				"Create a Camera3D node",
				schema, callable_mp_static(_tool_create_camera_3d));
	}

	// create_light_3d
	{
		Array light_types;
		light_types.push_back("DirectionalLight3D");
		light_types.push_back("OmniLight3D");
		light_types.push_back("SpotLight3D");

		Dictionary schema = MCPSchema::object()
									.prop("parent_path", "string", "Path to the parent node (default: /root)", true)
									.prop("name", "string", "Name for the light node", true)
									.prop_enum("light_type", light_types, "Type of light to create", true)
									.prop_object("properties", "Light properties (e.g., light_color, light_energy, shadow_enabled)")
									.build();

		p_server->register_tool("create_light_3d",
				"Create a 3D light node (DirectionalLight3D, OmniLight3D, SpotLight3D)",
				schema, callable_mp_static(_tool_create_light_3d));
	}

	// create_environment
	{
		Array sky_types;
		sky_types.push_back("procedural");
		sky_types.push_back("clear_color");
		sky_types.push_back("color");

		Dictionary schema = MCPSchema::object()
									.prop("parent_path", "string", "Path to the parent node (default: /root)", true)
									.prop("name", "string", "Name for the WorldEnvironment node", true)
									.prop_enum("sky_type", sky_types, "Type of sky/background (default: procedural)", false)
									.build();

		p_server->register_tool("create_environment",
				"Create a WorldEnvironment with Environment and Sky resources",
				schema, callable_mp_static(_tool_create_environment));
	}

	// create_csg_shape
	{
		Array shape_types;
		shape_types.push_back("CSGBox3D");
		shape_types.push_back("CSGSphere3D");
		shape_types.push_back("CSGCylinder3D");
		shape_types.push_back("CSGTorus3D");
		shape_types.push_back("CSGPolygon3D");
		shape_types.push_back("CSGMesh3D");
		shape_types.push_back("CSGCombiner3D");

		Dictionary schema = MCPSchema::object()
									.prop("parent_path", "string", "Path to the parent node (default: /root)", true)
									.prop("name", "string", "Name for the CSG node", true)
									.prop_enum("shape_type", shape_types, "Type of CSG shape to create", true)
									.prop_object("properties", "CSG shape properties (e.g., size, radius, height)")
									.build();

		p_server->register_tool("create_csg_shape",
				"Create a CSG shape node (CSGBox3D, CSGSphere3D, CSGCylinder3D, CSGTorus3D, CSGPolygon3D, CSGMesh3D, CSGCombiner3D)",
				schema, callable_mp_static(_tool_create_csg_shape));
	}
}
