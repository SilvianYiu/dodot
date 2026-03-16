/**************************************************************************/
/*  mcp_tools_materials.cpp                                               */
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

#include "mcp_tools_materials.h"

#include "../mcp_server.h"
#include "../mcp_tool_helpers.h"

#include "scene/3d/mesh_instance_3d.h"
#include "scene/resources/material.h"
#include "scene/resources/shader.h"

// ---------------------------------------------------------------------------
// create_standard_material
// ---------------------------------------------------------------------------

static Dictionary _tool_create_standard_material(const Dictionary &p_args) {
	String name = p_args.get("name", "StandardMaterial3D");
	Dictionary properties = p_args.get("properties", Dictionary());

	Ref<StandardMaterial3D> mat;
	mat.instantiate();

	// Apply color properties with special handling for albedo_color.
	for (const Variant *key = properties.next(nullptr); key; key = properties.next(key)) {
		String prop_name = *key;
		Variant value = properties[*key];

		if (prop_name == "albedo_color" && value.get_type() == Variant::STRING) {
			mat->set_albedo(Color::html(value));
		} else if (prop_name == "metallic") {
			mat->set_metallic(value);
		} else if (prop_name == "roughness") {
			mat->set_roughness(value);
		} else if (prop_name == "emission_enabled" && (bool)value) {
			mat->set_feature(BaseMaterial3D::FEATURE_EMISSION, true);
		} else if (prop_name == "emission" && value.get_type() == Variant::STRING) {
			mat->set_emission(Color::html(value));
		} else if (prop_name == "emission_energy") {
			mat->set_emission_energy_multiplier(value);
		} else if (prop_name == "transparency") {
			String mode = value;
			if (mode == "alpha") {
				mat->set_transparency(BaseMaterial3D::TRANSPARENCY_ALPHA);
			} else if (mode == "alpha_scissor") {
				mat->set_transparency(BaseMaterial3D::TRANSPARENCY_ALPHA_SCISSOR);
			} else if (mode == "alpha_hash") {
				mat->set_transparency(BaseMaterial3D::TRANSPARENCY_ALPHA_HASH);
			}
		} else {
			mat->set(StringName(prop_name), value);
		}
	}

	Dictionary result = mcp_success();
	result["material_name"] = name;
	result["material_type"] = "StandardMaterial3D";
	return result;
}

// ---------------------------------------------------------------------------
// apply_material
// ---------------------------------------------------------------------------

static Dictionary _tool_apply_material(const Dictionary &p_args) {
	String node_path = p_args.get("node_path", "");
	Dictionary material_properties = p_args.get("material_properties", Dictionary());

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

	Ref<StandardMaterial3D> mat;
	mat.instantiate();

	for (const Variant *key = material_properties.next(nullptr); key; key = material_properties.next(key)) {
		String prop_name = *key;
		Variant value = material_properties[*key];

		if (prop_name == "albedo_color" && value.get_type() == Variant::STRING) {
			mat->set_albedo(Color::html(value));
		} else if (prop_name == "metallic") {
			mat->set_metallic(value);
		} else if (prop_name == "roughness") {
			mat->set_roughness(value);
		} else if (prop_name == "emission_enabled" && (bool)value) {
			mat->set_feature(BaseMaterial3D::FEATURE_EMISSION, true);
		} else if (prop_name == "emission" && value.get_type() == Variant::STRING) {
			mat->set_emission(Color::html(value));
		} else {
			mat->set(StringName(prop_name), value);
		}
	}

	// Apply to surface 0 by default.
	mesh_instance->set_surface_override_material(0, mat);

	Dictionary result = mcp_success();
	result["node_path"] = node_path;
	result["material_type"] = "StandardMaterial3D";
	return result;
}

// ---------------------------------------------------------------------------
// create_shader_material
// ---------------------------------------------------------------------------

static Dictionary _tool_create_shader_material(const Dictionary &p_args) {
	String name = p_args.get("name", "ShaderMaterial");
	String shader_code = p_args.get("shader_code", "");

	if (shader_code.is_empty()) {
		return mcp_error("shader_code is required");
	}

	Ref<Shader> shader;
	shader.instantiate();
	shader->set_code(shader_code);

	Ref<ShaderMaterial> mat;
	mat.instantiate();
	mat->set_shader(shader);

	Dictionary result = mcp_success();
	result["material_name"] = name;
	result["material_type"] = "ShaderMaterial";
	return result;
}

// ---------------------------------------------------------------------------
// get_material_properties
// ---------------------------------------------------------------------------

static Dictionary _tool_get_material_properties(const Dictionary &p_args) {
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

	Dictionary result = mcp_success();
	result["node_path"] = node_path;

	int surface_count = 0;
	Ref<Mesh> mesh = mesh_instance->get_mesh();
	if (mesh.is_valid()) {
		surface_count = mesh->get_surface_count();
	}
	result["surface_count"] = surface_count;

	Array materials;
	for (int i = 0; i < surface_count; i++) {
		Dictionary mat_info;
		mat_info["surface_index"] = i;

		Ref<Material> mat = mesh_instance->get_surface_override_material(i);
		if (mat.is_null() && mesh.is_valid()) {
			mat = mesh->surface_get_material(i);
		}

		if (mat.is_valid()) {
			mat_info["type"] = mat->get_class();

			Ref<StandardMaterial3D> std_mat = mat;
			if (std_mat.is_valid()) {
				mat_info["albedo_color"] = std_mat->get_albedo().to_html();
				mat_info["metallic"] = std_mat->get_metallic();
				mat_info["roughness"] = std_mat->get_roughness();
				mat_info["emission_enabled"] = std_mat->get_feature(BaseMaterial3D::FEATURE_EMISSION);
				if (std_mat->get_feature(BaseMaterial3D::FEATURE_EMISSION)) {
					mat_info["emission"] = std_mat->get_emission().to_html();
					mat_info["emission_energy"] = std_mat->get_emission_energy_multiplier();
				}
			}
		} else {
			mat_info["type"] = "none";
		}

		materials.push_back(mat_info);
	}
	result["materials"] = materials;

	return result;
}

// ---------------------------------------------------------------------------
// Registration
// ---------------------------------------------------------------------------

void mcp_register_materials_tools(MCPServer *p_server) {
	// create_standard_material
	{
		Dictionary schema = MCPSchema::object()
									.prop("name", "string", "Name for the material resource", true)
									.prop_object("properties", "Material properties: albedo_color (hex string), metallic (0-1), roughness (0-1), emission_enabled (bool), emission (hex string), emission_energy (float), transparency (alpha/alpha_scissor/alpha_hash)")
									.build();

		p_server->register_tool("create_standard_material",
				"Create a StandardMaterial3D resource with specified properties",
				schema, callable_mp_static(_tool_create_standard_material));
	}

	// apply_material
	{
		Dictionary schema = MCPSchema::object()
									.prop("node_path", "string", "Path to the MeshInstance3D node", true)
									.prop_object("material_properties", "Material properties to apply: albedo_color (hex), metallic, roughness, emission_enabled, emission, etc.", true)
									.build();

		p_server->register_tool("apply_material",
				"Apply or modify a StandardMaterial3D on a MeshInstance3D node",
				schema, callable_mp_static(_tool_apply_material));
	}

	// create_shader_material
	{
		Dictionary schema = MCPSchema::object()
									.prop("name", "string", "Name for the shader material", true)
									.prop("shader_code", "string", "Godot shader language code", true)
									.build();

		p_server->register_tool("create_shader_material",
				"Create a ShaderMaterial with custom shader code",
				schema, callable_mp_static(_tool_create_shader_material));
	}

	// get_material_properties
	{
		Dictionary schema = MCPSchema::object()
									.prop("node_path", "string", "Path to the MeshInstance3D node", true)
									.build();

		p_server->register_tool("get_material_properties",
				"Get material properties from a MeshInstance3D node including albedo, metallic, roughness, and emission",
				schema, callable_mp_static(_tool_get_material_properties));
	}
}
