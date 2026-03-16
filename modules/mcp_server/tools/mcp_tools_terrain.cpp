/**************************************************************************/
/*  mcp_tools_terrain.cpp                                                 */
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

#include "mcp_tools_terrain.h"

#include "../mcp_server.h"
#include "../mcp_tool_helpers.h"

#include "scene/3d/light_3d.h"
#include "scene/3d/world_environment.h"
#include "scene/resources/3d/sky_material.h"
#include "scene/resources/environment.h"
#include "scene/resources/sky.h"

// --- Tool handlers ---

static Dictionary mcp_tool_setup_3d_environment(const Dictionary &p_args) {
	String parent_path = p_args.get("parent_path", "/root");
	String preset = p_args.get("preset", "outdoor_day");

	Dictionary err;
	Node *parent = mcp_get_or_create_parent(parent_path, err);
	if (!parent) {
		return err;
	}

	// Create WorldEnvironment.
	WorldEnvironment *world_env = memnew(WorldEnvironment);
	world_env->set_name("WorldEnvironment");

	Ref<Environment> env;
	env.instantiate();

	// Create sky.
	Ref<Sky> sky;
	sky.instantiate();
	Ref<ProceduralSkyMaterial> sky_mat;
	sky_mat.instantiate();

	// Configure based on preset.
	if (preset == "outdoor_day") {
		sky_mat->set_sky_top_color(Color(0.385, 0.454, 0.55));
		sky_mat->set_sky_horizon_color(Color(0.646, 0.654, 0.67));
		sky_mat->set_ground_bottom_color(Color(0.2, 0.169, 0.133));
		sky_mat->set_ground_horizon_color(Color(0.646, 0.654, 0.67));
		sky_mat->set_sun_angle_max(30.0);
		env->set_background(Environment::BG_SKY);
		env->set_ambient_source(Environment::AMBIENT_SOURCE_SKY);
		env->set_tonemapper(Environment::TONE_MAPPER_ACES);
		env->set_glow_enabled(false);
		env->set_ssao_enabled(true);
	} else if (preset == "outdoor_night") {
		sky_mat->set_sky_top_color(Color(0.02, 0.02, 0.05));
		sky_mat->set_sky_horizon_color(Color(0.05, 0.05, 0.1));
		sky_mat->set_ground_bottom_color(Color(0.02, 0.02, 0.02));
		sky_mat->set_ground_horizon_color(Color(0.05, 0.05, 0.1));
		env->set_background(Environment::BG_SKY);
		env->set_ambient_source(Environment::AMBIENT_SOURCE_SKY);
		env->set_tonemapper(Environment::TONE_MAPPER_ACES);
		env->set_glow_enabled(true);
	} else if (preset == "indoor") {
		env->set_background(Environment::BG_COLOR);
		env->set_bg_color(Color(0.3, 0.3, 0.3));
		env->set_ambient_source(Environment::AMBIENT_SOURCE_COLOR);
		env->set_ambient_light_color(Color(0.5, 0.5, 0.5));
		env->set_ambient_light_energy(0.5);
		env->set_ssao_enabled(true);
		env->set_sdfgi_enabled(false);
	} else if (preset == "space") {
		sky_mat->set_sky_top_color(Color(0.0, 0.0, 0.02));
		sky_mat->set_sky_horizon_color(Color(0.0, 0.0, 0.01));
		sky_mat->set_ground_bottom_color(Color(0.0, 0.0, 0.0));
		sky_mat->set_ground_horizon_color(Color(0.0, 0.0, 0.01));
		env->set_background(Environment::BG_SKY);
		env->set_ambient_source(Environment::AMBIENT_SOURCE_COLOR);
		env->set_ambient_light_color(Color(0.05, 0.05, 0.1));
		env->set_ambient_light_energy(0.2);
	} else {
		memdelete(world_env);
		return mcp_error("Unknown preset: " + preset + ". Use outdoor_day, outdoor_night, indoor, or space.");
	}

	sky->set_material(sky_mat);
	env->set_sky(sky);
	world_env->set_environment(env);
	mcp_add_child_to_scene(parent, world_env);

	// Create DirectionalLight3D for presets that need it.
	if (preset == "outdoor_day" || preset == "outdoor_night") {
		DirectionalLight3D *light = memnew(DirectionalLight3D);
		light->set_name("DirectionalLight3D");

		if (preset == "outdoor_day") {
			light->set_color(Color(1.0, 0.96, 0.9));
			light->set_param(Light3D::PARAM_ENERGY, 1.0);
			light->set_shadow(true);
			light->set_rotation(Vector3(Math::deg_to_rad(-50.0), Math::deg_to_rad(30.0), 0.0));
		} else {
			// outdoor_night: dim moonlight.
			light->set_color(Color(0.4, 0.45, 0.6));
			light->set_param(Light3D::PARAM_ENERGY, 0.15);
			light->set_shadow(true);
			light->set_rotation(Vector3(Math::deg_to_rad(-40.0), Math::deg_to_rad(150.0), 0.0));
		}

		mcp_add_child_to_scene(parent, light);
	}

	Dictionary result = mcp_success();
	result["preset"] = preset;
	result["world_environment"] = mcp_get_node_info(world_env);
	return result;
}

static Dictionary mcp_tool_create_directional_light(const Dictionary &p_args) {
	String parent_path = p_args.get("parent_path", "/root");
	String name = p_args.get("name", "DirectionalLight3D");
	Dictionary properties = p_args.get("properties", Dictionary());

	Dictionary err;
	Node *parent = mcp_get_or_create_parent(parent_path, err);
	if (!parent) {
		return err;
	}

	DirectionalLight3D *light = memnew(DirectionalLight3D);
	light->set_name(name);

	if (properties.has("color_r") || properties.has("color_g") || properties.has("color_b")) {
		light->set_color(Color(
				properties.get("color_r", 1.0),
				properties.get("color_g", 1.0),
				properties.get("color_b", 1.0)));
	}
	if (properties.has("energy")) {
		light->set_param(Light3D::PARAM_ENERGY, properties["energy"]);
	}
	if (properties.has("shadow")) {
		light->set_shadow(properties["shadow"]);
	}
	if (properties.has("shadow_bias")) {
		light->set_param(Light3D::PARAM_SHADOW_BIAS, properties["shadow_bias"]);
	}
	if (properties.has("rotation_x") || properties.has("rotation_y") || properties.has("rotation_z")) {
		light->set_rotation(Vector3(
				Math::deg_to_rad((double)properties.get("rotation_x", 0.0)),
				Math::deg_to_rad((double)properties.get("rotation_y", 0.0)),
				Math::deg_to_rad((double)properties.get("rotation_z", 0.0))));
	}
	if (properties.has("shadow_max_distance")) {
		light->set_param(Light3D::PARAM_SHADOW_MAX_DISTANCE, properties["shadow_max_distance"]);
	}

	mcp_add_child_to_scene(parent, light);

	Dictionary result = mcp_success();
	result["node"] = mcp_get_node_info(light);
	return result;
}

static Dictionary mcp_tool_create_sky(const Dictionary &p_args) {
	String environment_path = p_args.get("environment_path", "");
	String sky_type = p_args.get("sky_type", "procedural");
	Dictionary properties = p_args.get("properties", Dictionary());

	Dictionary err;
	Node *node = mcp_get_node(environment_path, err);
	if (!node) {
		return err;
	}

	WorldEnvironment *world_env = Object::cast_to<WorldEnvironment>(node);
	if (!world_env) {
		return mcp_error("Node is not a WorldEnvironment: " + environment_path);
	}

	Ref<Environment> env = world_env->get_environment();
	if (env.is_null()) {
		env.instantiate();
		world_env->set_environment(env);
	}

	Ref<Sky> sky;
	sky.instantiate();

	if (sky_type == "procedural") {
		Ref<ProceduralSkyMaterial> mat;
		mat.instantiate();

		if (properties.has("sky_top_color_r") || properties.has("sky_top_color_g") || properties.has("sky_top_color_b")) {
			mat->set_sky_top_color(Color(
					properties.get("sky_top_color_r", 0.385),
					properties.get("sky_top_color_g", 0.454),
					properties.get("sky_top_color_b", 0.55)));
		}
		if (properties.has("sky_horizon_color_r") || properties.has("sky_horizon_color_g") || properties.has("sky_horizon_color_b")) {
			mat->set_sky_horizon_color(Color(
					properties.get("sky_horizon_color_r", 0.646),
					properties.get("sky_horizon_color_g", 0.654),
					properties.get("sky_horizon_color_b", 0.67)));
		}
		if (properties.has("ground_bottom_color_r") || properties.has("ground_bottom_color_g") || properties.has("ground_bottom_color_b")) {
			mat->set_ground_bottom_color(Color(
					properties.get("ground_bottom_color_r", 0.2),
					properties.get("ground_bottom_color_g", 0.169),
					properties.get("ground_bottom_color_b", 0.133)));
		}
		if (properties.has("ground_horizon_color_r") || properties.has("ground_horizon_color_g") || properties.has("ground_horizon_color_b")) {
			mat->set_ground_horizon_color(Color(
					properties.get("ground_horizon_color_r", 0.646),
					properties.get("ground_horizon_color_g", 0.654),
					properties.get("ground_horizon_color_b", 0.67)));
		}
		if (properties.has("sun_angle_max")) {
			mat->set_sun_angle_max(properties["sun_angle_max"]);
		}
		if (properties.has("sun_curve")) {
			mat->set_sun_curve(properties["sun_curve"]);
		}

		sky->set_material(mat);
	} else if (sky_type == "physical") {
		Ref<PhysicalSkyMaterial> mat;
		mat.instantiate();

		if (properties.has("rayleigh_coefficient")) {
			mat->set_rayleigh_coefficient(properties["rayleigh_coefficient"]);
		}
		if (properties.has("mie_coefficient")) {
			mat->set_mie_coefficient(properties["mie_coefficient"]);
		}
		if (properties.has("turbidity")) {
			mat->set_turbidity(properties["turbidity"]);
		}
		if (properties.has("sun_disk_scale")) {
			mat->set_sun_disk_scale(properties["sun_disk_scale"]);
		}
		if (properties.has("ground_color_r") || properties.has("ground_color_g") || properties.has("ground_color_b")) {
			mat->set_ground_color(Color(
					properties.get("ground_color_r", 0.1),
					properties.get("ground_color_g", 0.07),
					properties.get("ground_color_b", 0.034)));
		}

		sky->set_material(mat);
	} else {
		return mcp_error("Unknown sky type: " + sky_type + ". Use procedural or physical.");
	}

	env->set_sky(sky);
	env->set_background(Environment::BG_SKY);

	Dictionary result = mcp_success();
	result["message"] = "Sky configured for: " + environment_path;
	result["sky_type"] = sky_type;
	return result;
}

static Dictionary mcp_tool_create_fog(const Dictionary &p_args) {
	String environment_path = p_args.get("environment_path", "");
	Dictionary properties = p_args.get("properties", Dictionary());

	Dictionary err;
	Node *node = mcp_get_node(environment_path, err);
	if (!node) {
		return err;
	}

	WorldEnvironment *world_env = Object::cast_to<WorldEnvironment>(node);
	if (!world_env) {
		return mcp_error("Node is not a WorldEnvironment: " + environment_path);
	}

	Ref<Environment> env = world_env->get_environment();
	if (env.is_null()) {
		env.instantiate();
		world_env->set_environment(env);
	}

	// Configure standard fog.
	bool use_volumetric = properties.get("volumetric", false);

	if (use_volumetric) {
		env->set_volumetric_fog_enabled(true);
		if (properties.has("density")) {
			env->set_volumetric_fog_density(properties["density"]);
		}
		if (properties.has("albedo_r") || properties.has("albedo_g") || properties.has("albedo_b")) {
			env->set_volumetric_fog_albedo(Color(
					properties.get("albedo_r", 1.0),
					properties.get("albedo_g", 1.0),
					properties.get("albedo_b", 1.0)));
		}
		if (properties.has("emission_r") || properties.has("emission_g") || properties.has("emission_b")) {
			env->set_volumetric_fog_emission(Color(
					properties.get("emission_r", 0.0),
					properties.get("emission_g", 0.0),
					properties.get("emission_b", 0.0)));
		}
		if (properties.has("emission_energy")) {
			env->set_volumetric_fog_emission_energy(properties["emission_energy"]);
		}
		if (properties.has("anisotropy")) {
			env->set_volumetric_fog_anisotropy(properties["anisotropy"]);
		}
		if (properties.has("length")) {
			env->set_volumetric_fog_length(properties["length"]);
		}
	} else {
		env->set_fog_enabled(true);
		if (properties.has("color_r") || properties.has("color_g") || properties.has("color_b")) {
			env->set_fog_light_color(Color(
					properties.get("color_r", 0.5),
					properties.get("color_g", 0.6),
					properties.get("color_b", 0.7)));
		}
		if (properties.has("density")) {
			env->set_fog_density(properties["density"]);
		}
		if (properties.has("sky_affect")) {
			env->set_fog_sky_affect(properties["sky_affect"]);
		}
		if (properties.has("height")) {
			env->set_fog_height(properties["height"]);
		}
		if (properties.has("height_density")) {
			env->set_fog_height_density(properties["height_density"]);
		}
		if (properties.has("aerial_perspective")) {
			env->set_fog_aerial_perspective(properties["aerial_perspective"]);
		}
	}

	Dictionary result = mcp_success();
	result["message"] = "Fog configured for: " + environment_path;
	result["volumetric"] = use_volumetric;
	return result;
}

// --- Registration ---

void mcp_register_terrain_tools(MCPServer *p_server) {
	// setup_3d_environment
	{
		Array presets;
		presets.push_back("outdoor_day");
		presets.push_back("outdoor_night");
		presets.push_back("indoor");
		presets.push_back("space");

		Dictionary schema = MCPSchema::object()
									.prop("parent_path", "string", "Parent node path", true)
									.prop_enum("preset", presets, "Environment preset", true)
									.build();

		p_server->register_tool("setup_3d_environment",
				"One-shot 3D environment setup: WorldEnvironment + DirectionalLight3D + sky with preset",
				schema, callable_mp_static(&mcp_tool_setup_3d_environment));
	}

	// create_directional_light
	{
		Dictionary schema = MCPSchema::object()
									.prop("parent_path", "string", "Parent node path", true)
									.prop("name", "string", "Node name")
									.prop_object("properties", "Light properties: color_r/g/b, energy, shadow(bool), shadow_bias, rotation_x/y/z (degrees), shadow_max_distance")
									.build();

		p_server->register_tool("create_directional_light",
				"Create a DirectionalLight3D with shadow configuration",
				schema, callable_mp_static(&mcp_tool_create_directional_light));
	}

	// create_sky
	{
		Array sky_types;
		sky_types.push_back("procedural");
		sky_types.push_back("physical");

		Dictionary schema = MCPSchema::object()
									.prop("environment_path", "string", "Path to WorldEnvironment node", true)
									.prop_enum("sky_type", sky_types, "Sky material type", true)
									.prop_object("properties", "Sky properties. Procedural: sky_top_color_r/g/b, sky_horizon_color_r/g/b, ground_bottom_color_r/g/b, ground_horizon_color_r/g/b, sun_angle_max, sun_curve. Physical: rayleigh_coefficient, mie_coefficient, turbidity, sun_disk_scale, ground_color_r/g/b")
									.build();

		p_server->register_tool("create_sky",
				"Configure sky on a WorldEnvironment",
				schema, callable_mp_static(&mcp_tool_create_sky));
	}

	// create_fog
	{
		Dictionary schema = MCPSchema::object()
									.prop("environment_path", "string", "Path to WorldEnvironment node", true)
									.prop_object("properties", "Fog properties. Standard: color_r/g/b, density, sky_affect, height, height_density, aerial_perspective. Volumetric (set volumetric=true): density, albedo_r/g/b, emission_r/g/b, emission_energy, anisotropy, length", true)
									.build();

		p_server->register_tool("create_fog",
				"Configure fog (standard or volumetric) on a WorldEnvironment",
				schema, callable_mp_static(&mcp_tool_create_fog));
	}
}
