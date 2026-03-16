/**************************************************************************/
/*  mcp_tools_particles.cpp                                               */
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

#include "mcp_tools_particles.h"

#include "../mcp_server.h"
#include "../mcp_tool_helpers.h"

#include "scene/3d/cpu_particles_3d.h"
#include "scene/3d/gpu_particles_3d.h"
#include "scene/resources/particle_process_material.h"

// --- Tool handlers ---

static Dictionary mcp_tool_create_gpu_particles_3d(const Dictionary &p_args) {
	String parent_path = p_args.get("parent_path", "/root");
	String name = p_args.get("name", "GPUParticles3D");
	Dictionary properties = p_args.get("properties", Dictionary());

	Dictionary err;
	Node *parent = mcp_get_or_create_parent(parent_path, err);
	if (!parent) {
		return err;
	}

	GPUParticles3D *particles = memnew(GPUParticles3D);
	particles->set_name(name);

	// Create and assign a ParticleProcessMaterial.
	Ref<ParticleProcessMaterial> mat;
	mat.instantiate();

	// Apply material properties from input.
	if (properties.has("direction_x") || properties.has("direction_y") || properties.has("direction_z")) {
		mat->set_direction(Vector3(
				properties.get("direction_x", 0.0),
				properties.get("direction_y", 1.0),
				properties.get("direction_z", 0.0)));
	}
	if (properties.has("spread")) {
		mat->set_spread(properties["spread"]);
	}
	if (properties.has("gravity_x") || properties.has("gravity_y") || properties.has("gravity_z")) {
		mat->set_gravity(Vector3(
				properties.get("gravity_x", 0.0),
				properties.get("gravity_y", -9.8),
				properties.get("gravity_z", 0.0)));
	}
	if (properties.has("initial_velocity_min")) {
		mat->set_param_min(ParticleProcessMaterial::PARAM_INITIAL_LINEAR_VELOCITY, properties["initial_velocity_min"]);
	}
	if (properties.has("initial_velocity_max")) {
		mat->set_param_max(ParticleProcessMaterial::PARAM_INITIAL_LINEAR_VELOCITY, properties["initial_velocity_max"]);
	}
	if (properties.has("scale_min")) {
		mat->set_param_min(ParticleProcessMaterial::PARAM_SCALE, properties["scale_min"]);
	}
	if (properties.has("scale_max")) {
		mat->set_param_max(ParticleProcessMaterial::PARAM_SCALE, properties["scale_max"]);
	}

	particles->set_process_material(mat);

	// Apply particle node properties.
	if (properties.has("amount")) {
		particles->set_amount(properties["amount"]);
	}
	if (properties.has("lifetime")) {
		particles->set_lifetime(properties["lifetime"]);
	}
	if (properties.has("one_shot")) {
		particles->set_one_shot(properties["one_shot"]);
	}
	if (properties.has("explosiveness")) {
		particles->set_explosiveness_ratio(properties["explosiveness"]);
	}
	if (properties.has("emitting")) {
		particles->set_emitting(properties["emitting"]);
	}

	mcp_add_child_to_scene(parent, particles);

	Dictionary result = mcp_success();
	result["node"] = mcp_get_node_info(particles);
	return result;
}

static Dictionary mcp_tool_create_cpu_particles_3d(const Dictionary &p_args) {
	String parent_path = p_args.get("parent_path", "/root");
	String name = p_args.get("name", "CPUParticles3D");
	Dictionary properties = p_args.get("properties", Dictionary());

	Dictionary err;
	Node *parent = mcp_get_or_create_parent(parent_path, err);
	if (!parent) {
		return err;
	}

	CPUParticles3D *particles = memnew(CPUParticles3D);
	particles->set_name(name);

	// Apply properties.
	if (properties.has("amount")) {
		particles->set_amount(properties["amount"]);
	}
	if (properties.has("lifetime")) {
		particles->set_lifetime(properties["lifetime"]);
	}
	if (properties.has("one_shot")) {
		particles->set_one_shot(properties["one_shot"]);
	}
	if (properties.has("explosiveness")) {
		particles->set_explosiveness_ratio(properties["explosiveness"]);
	}
	if (properties.has("direction_x") || properties.has("direction_y") || properties.has("direction_z")) {
		particles->set_direction(Vector3(
				properties.get("direction_x", 0.0),
				properties.get("direction_y", 1.0),
				properties.get("direction_z", 0.0)));
	}
	if (properties.has("spread")) {
		particles->set_spread(properties["spread"]);
	}
	if (properties.has("gravity_x") || properties.has("gravity_y") || properties.has("gravity_z")) {
		particles->set_gravity(Vector3(
				properties.get("gravity_x", 0.0),
				properties.get("gravity_y", -9.8),
				properties.get("gravity_z", 0.0)));
	}
	if (properties.has("initial_velocity_min")) {
		particles->set_param_min(CPUParticles3D::PARAM_INITIAL_LINEAR_VELOCITY, properties["initial_velocity_min"]);
	}
	if (properties.has("initial_velocity_max")) {
		particles->set_param_max(CPUParticles3D::PARAM_INITIAL_LINEAR_VELOCITY, properties["initial_velocity_max"]);
	}
	if (properties.has("emitting")) {
		particles->set_emitting(properties["emitting"]);
	}

	mcp_add_child_to_scene(parent, particles);

	Dictionary result = mcp_success();
	result["node"] = mcp_get_node_info(particles);
	return result;
}

static Dictionary mcp_tool_configure_particle_material(const Dictionary &p_args) {
	String node_path = p_args.get("node_path", "");
	Dictionary properties = p_args.get("properties", Dictionary());

	Dictionary err;
	Node *node = mcp_get_node(node_path, err);
	if (!node) {
		return err;
	}

	GPUParticles3D *gpu_particles = Object::cast_to<GPUParticles3D>(node);
	if (!gpu_particles) {
		return mcp_error("Node is not a GPUParticles3D: " + node_path);
	}

	Ref<ParticleProcessMaterial> mat = gpu_particles->get_process_material();
	if (mat.is_null()) {
		mat.instantiate();
		gpu_particles->set_process_material(mat);
	}

	// Direction and spread.
	if (properties.has("direction_x") || properties.has("direction_y") || properties.has("direction_z")) {
		Vector3 current_dir = mat->get_direction();
		mat->set_direction(Vector3(
				properties.get("direction_x", current_dir.x),
				properties.get("direction_y", current_dir.y),
				properties.get("direction_z", current_dir.z)));
	}
	if (properties.has("spread")) {
		mat->set_spread(properties["spread"]);
	}

	// Gravity.
	if (properties.has("gravity_x") || properties.has("gravity_y") || properties.has("gravity_z")) {
		Vector3 current_grav = mat->get_gravity();
		mat->set_gravity(Vector3(
				properties.get("gravity_x", current_grav.x),
				properties.get("gravity_y", current_grav.y),
				properties.get("gravity_z", current_grav.z)));
	}

	// Velocity.
	if (properties.has("initial_velocity_min")) {
		mat->set_param_min(ParticleProcessMaterial::PARAM_INITIAL_LINEAR_VELOCITY, properties["initial_velocity_min"]);
	}
	if (properties.has("initial_velocity_max")) {
		mat->set_param_max(ParticleProcessMaterial::PARAM_INITIAL_LINEAR_VELOCITY, properties["initial_velocity_max"]);
	}

	// Scale.
	if (properties.has("scale_min")) {
		mat->set_param_min(ParticleProcessMaterial::PARAM_SCALE, properties["scale_min"]);
	}
	if (properties.has("scale_max")) {
		mat->set_param_max(ParticleProcessMaterial::PARAM_SCALE, properties["scale_max"]);
	}

	// Angular velocity.
	if (properties.has("angular_velocity_min")) {
		mat->set_param_min(ParticleProcessMaterial::PARAM_ANGULAR_VELOCITY, properties["angular_velocity_min"]);
	}
	if (properties.has("angular_velocity_max")) {
		mat->set_param_max(ParticleProcessMaterial::PARAM_ANGULAR_VELOCITY, properties["angular_velocity_max"]);
	}

	// Emission shape.
	if (properties.has("emission_shape")) {
		String shape_str = properties["emission_shape"];
		if (shape_str == "point") {
			mat->set_emission_shape(ParticleProcessMaterial::EMISSION_SHAPE_POINT);
		} else if (shape_str == "sphere") {
			mat->set_emission_shape(ParticleProcessMaterial::EMISSION_SHAPE_SPHERE);
		} else if (shape_str == "sphere_surface") {
			mat->set_emission_shape(ParticleProcessMaterial::EMISSION_SHAPE_SPHERE_SURFACE);
		} else if (shape_str == "box") {
			mat->set_emission_shape(ParticleProcessMaterial::EMISSION_SHAPE_BOX);
		} else if (shape_str == "ring") {
			mat->set_emission_shape(ParticleProcessMaterial::EMISSION_SHAPE_RING);
		}
	}
	if (properties.has("emission_sphere_radius")) {
		mat->set_emission_sphere_radius(properties["emission_sphere_radius"]);
	}
	if (properties.has("emission_box_extents_x") || properties.has("emission_box_extents_y") || properties.has("emission_box_extents_z")) {
		mat->set_emission_box_extents(Vector3(
				properties.get("emission_box_extents_x", 1.0),
				properties.get("emission_box_extents_y", 1.0),
				properties.get("emission_box_extents_z", 1.0)));
	}

	// Color.
	if (properties.has("color_r") || properties.has("color_g") || properties.has("color_b")) {
		mat->set_color(Color(
				properties.get("color_r", 1.0),
				properties.get("color_g", 1.0),
				properties.get("color_b", 1.0),
				properties.get("color_a", 1.0)));
	}

	Dictionary result = mcp_success();
	result["message"] = "Particle material configured for: " + node_path;
	return result;
}

// --- Registration ---

void mcp_register_particles_tools(MCPServer *p_server) {
	// create_gpu_particles_3d
	{
		Dictionary schema = MCPSchema::object()
									.prop("parent_path", "string", "Parent node path", true)
									.prop("name", "string", "Node name")
									.prop_object("properties", "Particle properties: amount, lifetime, one_shot, explosiveness, emitting, direction_x/y/z, spread, gravity_x/y/z, initial_velocity_min/max, scale_min/max")
									.build();

		p_server->register_tool("create_gpu_particles_3d",
				"Create a GPUParticles3D with a ParticleProcessMaterial",
				schema, callable_mp_static(&mcp_tool_create_gpu_particles_3d));
	}

	// create_cpu_particles_3d
	{
		Dictionary schema = MCPSchema::object()
									.prop("parent_path", "string", "Parent node path", true)
									.prop("name", "string", "Node name")
									.prop_object("properties", "Particle properties: amount, lifetime, one_shot, explosiveness, emitting, direction_x/y/z, spread, gravity_x/y/z, initial_velocity_min/max")
									.build();

		p_server->register_tool("create_cpu_particles_3d",
				"Create a CPUParticles3D node",
				schema, callable_mp_static(&mcp_tool_create_cpu_particles_3d));
	}

	// configure_particle_material
	{
		Dictionary schema = MCPSchema::object()
									.prop("node_path", "string", "Path to GPUParticles3D node", true)
									.prop_object("properties", "Material properties: direction_x/y/z, spread, gravity_x/y/z, initial_velocity_min/max, scale_min/max, angular_velocity_min/max, emission_shape, emission_sphere_radius, emission_box_extents_x/y/z, color_r/g/b/a", true)
									.build();

		p_server->register_tool("configure_particle_material",
				"Modify particle material properties (emission shape, gravity, velocity, color, etc.)",
				schema, callable_mp_static(&mcp_tool_configure_particle_material));
	}
}
