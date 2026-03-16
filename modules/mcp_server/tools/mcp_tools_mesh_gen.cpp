/**************************************************************************/
/*  mcp_tools_mesh_gen.cpp                                                */
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

#include "mcp_tools_mesh_gen.h"

#include "../mcp_server.h"
#include "../mcp_tool_helpers.h"

#include "scene/3d/mesh_instance_3d.h"
#include "scene/resources/3d/primitive_meshes.h"

// ---------------------------------------------------------------------------
// create_primitive_mesh
// ---------------------------------------------------------------------------

static Dictionary _tool_create_primitive_mesh(const Dictionary &p_args) {
	String type = p_args.get("type", "BoxMesh");
	Dictionary properties = p_args.get("properties", Dictionary());

	Ref<PrimitiveMesh> mesh;

	if (type == "BoxMesh") {
		Ref<BoxMesh> box;
		box.instantiate();
		if (properties.has("size_x") || properties.has("size_y") || properties.has("size_z")) {
			Vector3 size;
			size.x = properties.get("size_x", 1.0);
			size.y = properties.get("size_y", 1.0);
			size.z = properties.get("size_z", 1.0);
			box->set_size(size);
		}
		if (properties.has("subdivide_width")) {
			box->set_subdivide_width(properties["subdivide_width"]);
		}
		if (properties.has("subdivide_height")) {
			box->set_subdivide_height(properties["subdivide_height"]);
		}
		if (properties.has("subdivide_depth")) {
			box->set_subdivide_depth(properties["subdivide_depth"]);
		}
		mesh = box;
	} else if (type == "SphereMesh") {
		Ref<SphereMesh> sphere;
		sphere.instantiate();
		if (properties.has("radius")) {
			sphere->set_radius(properties["radius"]);
		}
		if (properties.has("height")) {
			sphere->set_height(properties["height"]);
		}
		if (properties.has("radial_segments")) {
			sphere->set_radial_segments(properties["radial_segments"]);
		}
		if (properties.has("rings")) {
			sphere->set_rings(properties["rings"]);
		}
		if (properties.has("is_hemisphere")) {
			sphere->set_is_hemisphere(properties["is_hemisphere"]);
		}
		mesh = sphere;
	} else if (type == "CylinderMesh") {
		Ref<CylinderMesh> cylinder;
		cylinder.instantiate();
		if (properties.has("top_radius")) {
			cylinder->set_top_radius(properties["top_radius"]);
		}
		if (properties.has("bottom_radius")) {
			cylinder->set_bottom_radius(properties["bottom_radius"]);
		}
		if (properties.has("height")) {
			cylinder->set_height(properties["height"]);
		}
		if (properties.has("radial_segments")) {
			cylinder->set_radial_segments(properties["radial_segments"]);
		}
		if (properties.has("rings")) {
			cylinder->set_rings(properties["rings"]);
		}
		mesh = cylinder;
	} else if (type == "PlaneMesh") {
		Ref<PlaneMesh> plane;
		plane.instantiate();
		if (properties.has("size_x") || properties.has("size_y")) {
			Size2 size;
			size.x = properties.get("size_x", 2.0);
			size.y = properties.get("size_y", 2.0);
			plane->set_size(size);
		}
		if (properties.has("subdivide_width")) {
			plane->set_subdivide_width(properties["subdivide_width"]);
		}
		if (properties.has("subdivide_depth")) {
			plane->set_subdivide_depth(properties["subdivide_depth"]);
		}
		mesh = plane;
	} else if (type == "CapsuleMesh") {
		Ref<CapsuleMesh> capsule;
		capsule.instantiate();
		if (properties.has("radius")) {
			capsule->set_radius(properties["radius"]);
		}
		if (properties.has("height")) {
			capsule->set_height(properties["height"]);
		}
		if (properties.has("radial_segments")) {
			capsule->set_radial_segments(properties["radial_segments"]);
		}
		if (properties.has("rings")) {
			capsule->set_rings(properties["rings"]);
		}
		mesh = capsule;
	} else if (type == "PrismMesh") {
		Ref<PrismMesh> prism;
		prism.instantiate();
		if (properties.has("left_to_right")) {
			prism->set_left_to_right(properties["left_to_right"]);
		}
		if (properties.has("size_x") || properties.has("size_y") || properties.has("size_z")) {
			Vector3 size;
			size.x = properties.get("size_x", 1.0);
			size.y = properties.get("size_y", 1.0);
			size.z = properties.get("size_z", 1.0);
			prism->set_size(size);
		}
		if (properties.has("subdivide_width")) {
			prism->set_subdivide_width(properties["subdivide_width"]);
		}
		if (properties.has("subdivide_height")) {
			prism->set_subdivide_height(properties["subdivide_height"]);
		}
		if (properties.has("subdivide_depth")) {
			prism->set_subdivide_depth(properties["subdivide_depth"]);
		}
		mesh = prism;
	} else if (type == "TorusMesh") {
		Ref<TorusMesh> torus;
		torus.instantiate();
		if (properties.has("inner_radius")) {
			torus->set_inner_radius(properties["inner_radius"]);
		}
		if (properties.has("outer_radius")) {
			torus->set_outer_radius(properties["outer_radius"]);
		}
		if (properties.has("rings")) {
			torus->set_rings(properties["rings"]);
		}
		if (properties.has("ring_segments")) {
			torus->set_ring_segments(properties["ring_segments"]);
		}
		mesh = torus;
	} else {
		return mcp_error("Unknown mesh type: " + type + ". Supported: BoxMesh, SphereMesh, CylinderMesh, PlaneMesh, CapsuleMesh, PrismMesh, TorusMesh");
	}

	Dictionary result = mcp_success();
	result["mesh_type"] = type;

	// Report mesh info.
	AABB aabb = mesh->get_aabb();
	Dictionary aabb_info;
	aabb_info["position_x"] = aabb.position.x;
	aabb_info["position_y"] = aabb.position.y;
	aabb_info["position_z"] = aabb.position.z;
	aabb_info["size_x"] = aabb.size.x;
	aabb_info["size_y"] = aabb.size.y;
	aabb_info["size_z"] = aabb.size.z;
	result["aabb"] = aabb_info;
	result["surface_count"] = mesh->get_surface_count();

	return result;
}

// ---------------------------------------------------------------------------
// get_mesh_info
// ---------------------------------------------------------------------------

static Dictionary _tool_get_mesh_info(const Dictionary &p_args) {
	String node_path = p_args.get("node_path", "");

	if (node_path.is_empty()) {
		return mcp_error("node_path is required");
	}

	Dictionary err;
	Node *node = mcp_get_node(node_path, err);
	if (!node) {
		return err;
	}

	MeshInstance3D *mesh_instance = Object::cast_to<MeshInstance3D>(node);
	if (!mesh_instance) {
		return mcp_error("Node is not a MeshInstance3D: " + node_path);
	}

	Ref<Mesh> mesh = mesh_instance->get_mesh();
	if (mesh.is_null()) {
		return mcp_error("MeshInstance3D has no mesh assigned: " + node_path);
	}

	Dictionary result = mcp_success();
	result["node_path"] = node_path;
	result["mesh_class"] = mesh->get_class();
	result["surface_count"] = mesh->get_surface_count();

	AABB aabb = mesh->get_aabb();
	Dictionary aabb_info;
	aabb_info["position_x"] = aabb.position.x;
	aabb_info["position_y"] = aabb.position.y;
	aabb_info["position_z"] = aabb.position.z;
	aabb_info["size_x"] = aabb.size.x;
	aabb_info["size_y"] = aabb.size.y;
	aabb_info["size_z"] = aabb.size.z;
	result["aabb"] = aabb_info;

	Array surfaces;
	for (int i = 0; i < mesh->get_surface_count(); i++) {
		Dictionary surf_info;
		surf_info["index"] = i;
		surf_info["primitive_type"] = mesh->surface_get_primitive_type(i);

		int vertex_count = 0;
		Array arrays = mesh->surface_get_arrays(i);
		if (arrays.size() > 0 && arrays[Mesh::ARRAY_VERTEX].get_type() == Variant::PACKED_VECTOR3_ARRAY) {
			PackedVector3Array verts = arrays[Mesh::ARRAY_VERTEX];
			vertex_count = verts.size();
		}
		surf_info["vertex_count"] = vertex_count;

		Ref<Material> mat = mesh->surface_get_material(i);
		if (mat.is_valid()) {
			surf_info["material_type"] = mat->get_class();
		} else {
			surf_info["material_type"] = "none";
		}

		surfaces.push_back(surf_info);
	}
	result["surfaces"] = surfaces;

	return result;
}

// ---------------------------------------------------------------------------
// Registration
// ---------------------------------------------------------------------------

void mcp_register_mesh_gen_tools(MCPServer *p_server) {
	// create_primitive_mesh
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
									.prop_enum("type", mesh_types, "Type of primitive mesh to create", true)
									.prop_object("properties", "Mesh-specific properties. BoxMesh: size_x/y/z, subdivide_width/height/depth. SphereMesh: radius, height, radial_segments, rings, is_hemisphere. CylinderMesh: top_radius, bottom_radius, height, radial_segments, rings. PlaneMesh: size_x, size_y, subdivide_width, subdivide_depth. CapsuleMesh: radius, height, radial_segments, rings. PrismMesh: left_to_right, size_x/y/z, subdivide_width/height/depth. TorusMesh: inner_radius, outer_radius, rings, ring_segments.")
									.build();

		p_server->register_tool("create_primitive_mesh",
				"Create a primitive mesh with full property control (size, radius, height, rings, segments, etc.)",
				schema, callable_mp_static(_tool_create_primitive_mesh));
	}

	// get_mesh_info
	{
		Dictionary schema = MCPSchema::object()
									.prop("node_path", "string", "Path to the MeshInstance3D node", true)
									.build();

		p_server->register_tool("get_mesh_info",
				"Get mesh details from a MeshInstance3D including vertex count, surface count, AABB, and material info",
				schema, callable_mp_static(_tool_get_mesh_info));
	}
}
