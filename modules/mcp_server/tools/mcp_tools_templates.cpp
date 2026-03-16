/**************************************************************************/
/*  mcp_tools_templates.cpp                                               */
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

#include "mcp_tools_templates.h"

#include "../mcp_server.h"
#include "../mcp_tool_helpers.h"

#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/version.h"

// ---------------------------------------------------------------------------
// Template content generators
// ---------------------------------------------------------------------------

static void _write_file(const String &p_path, const String &p_content) {
	Ref<FileAccess> file = FileAccess::open(p_path, FileAccess::WRITE);
	if (file.is_valid()) {
		file->store_string(p_content);
		file->close();
	}
}

static void _ensure_dir(const String &p_path) {
	Ref<DirAccess> dir = DirAccess::open("res://");
	if (dir.is_valid()) {
		dir->make_dir_recursive(p_path);
	}
}

static String _make_project_godot(const String &p_name, const String &p_main_scene) {
	String content;
	content += "; Engine configuration file.\n";
	content += "; It's best edited using the editor UI and not directly,\n";
	content += "; since the parameters that go here are not all obvious.\n\n";
	content += "[application]\n\n";
	content += "config/name=\"" + p_name + "\"\n";
	content += "run/main_scene=\"" + p_main_scene + "\"\n";
	content += "config/features=PackedStringArray(\"" + String(VERSION_BRANCH) + "\")\n";
	return content;
}

static String _make_2d_platformer_player_script() {
	return R"(extends CharacterBody2D

const SPEED: float = 300.0
const JUMP_VELOCITY: float = -400.0

func _physics_process(delta: float) -> void:
	# Add gravity.
	if not is_on_floor():
		velocity += get_gravity() * delta

	# Handle jump.
	if Input.is_action_just_pressed("ui_accept") and is_on_floor():
		velocity.y = JUMP_VELOCITY

	# Horizontal movement.
	var direction: float = Input.get_axis("ui_left", "ui_right")
	if direction:
		velocity.x = direction * SPEED
	else:
		velocity.x = move_toward(velocity.x, 0, SPEED)

	move_and_slide()
)";
}

static String _make_3d_fps_player_script() {
	return R"(extends CharacterBody3D

const SPEED: float = 5.0
const JUMP_VELOCITY: float = 4.5
const MOUSE_SENSITIVITY: float = 0.002

@onready var camera: Camera3D = $Camera3D

func _ready() -> void:
	Input.set_mouse_mode(Input.MOUSE_MODE_CAPTURED)

func _unhandled_input(event: InputEvent) -> void:
	if event is InputEventMouseMotion:
		rotate_y(-event.relative.x * MOUSE_SENSITIVITY)
		camera.rotate_x(-event.relative.y * MOUSE_SENSITIVITY)
		camera.rotation.x = clamp(camera.rotation.x, -PI / 2, PI / 2)

	if event.is_action_pressed("ui_cancel"):
		Input.set_mouse_mode(Input.MOUSE_MODE_VISIBLE)

func _physics_process(delta: float) -> void:
	if not is_on_floor():
		velocity += get_gravity() * delta

	if Input.is_action_just_pressed("ui_accept") and is_on_floor():
		velocity.y = JUMP_VELOCITY

	var input_dir: Vector2 = Input.get_vector("ui_left", "ui_right", "ui_up", "ui_down")
	var direction: Vector3 = (transform.basis * Vector3(input_dir.x, 0, input_dir.y)).normalized()
	if direction:
		velocity.x = direction.x * SPEED
		velocity.z = direction.z * SPEED
	else:
		velocity.x = move_toward(velocity.x, 0, SPEED)
		velocity.z = move_toward(velocity.z, 0, SPEED)

	move_and_slide()
)";
}

static String _make_3d_third_person_player_script() {
	return R"(extends CharacterBody3D

const SPEED: float = 5.0
const JUMP_VELOCITY: float = 4.5
const ROTATION_SPEED: float = 5.0

func _physics_process(delta: float) -> void:
	if not is_on_floor():
		velocity += get_gravity() * delta

	if Input.is_action_just_pressed("ui_accept") and is_on_floor():
		velocity.y = JUMP_VELOCITY

	var input_dir: Vector2 = Input.get_vector("ui_left", "ui_right", "ui_up", "ui_down")
	var direction: Vector3 = (Vector3(input_dir.x, 0, input_dir.y)).normalized()

	if direction:
		velocity.x = direction.x * SPEED
		velocity.z = direction.z * SPEED
		# Rotate to face movement direction.
		var target_angle: float = atan2(direction.x, direction.z)
		rotation.y = lerp_angle(rotation.y, target_angle, ROTATION_SPEED * delta)
	else:
		velocity.x = move_toward(velocity.x, 0, SPEED)
		velocity.z = move_toward(velocity.z, 0, SPEED)

	move_and_slide()
)";
}

static String _make_ui_app_main_script() {
	return R"(extends Control

func _ready() -> void:
	pass

func _on_button_pressed() -> void:
	print("Button pressed!")
)";
}

static String _make_empty_2d_script() {
	return R"(extends Node2D

func _ready() -> void:
	pass

func _process(delta: float) -> void:
	pass
)";
}

static String _make_empty_3d_script() {
	return R"(extends Node3D

func _ready() -> void:
	pass

func _process(delta: float) -> void:
	pass
)";
}

// ---------------------------------------------------------------------------
// create_project_template
// ---------------------------------------------------------------------------

static Dictionary _tool_create_project_template(const Dictionary &p_args) {
	String template_name = p_args.get("template_name", "");
	String project_path = p_args.get("project_path", "");
	Dictionary config = p_args.get("config", Dictionary());

	if (template_name.is_empty() || project_path.is_empty()) {
		return mcp_error("template_name and project_path are required");
	}

	// Validate template name.
	if (template_name != "2d_platformer" && template_name != "3d_fps" &&
			template_name != "3d_third_person" && template_name != "ui_app" &&
			template_name != "empty_2d" && template_name != "empty_3d") {
		return mcp_error("Unknown template: " + template_name + ". Available: 2d_platformer, 3d_fps, 3d_third_person, ui_app, empty_2d, empty_3d");
	}

	String project_name = config.get("project_name", template_name);

	// Create directory structure.
	_ensure_dir(project_path);
	_ensure_dir(project_path.path_join("scenes"));
	_ensure_dir(project_path.path_join("scripts"));
	_ensure_dir(project_path.path_join("assets"));
	_ensure_dir(project_path.path_join("assets/textures"));
	_ensure_dir(project_path.path_join("assets/audio"));

	Array created_files;
	String main_scene;

	if (template_name == "2d_platformer") {
		main_scene = "res://scenes/main.tscn";

		String scene = "[gd_scene load_steps=2 format=3]\n\n";
		scene += "[ext_resource type=\"Script\" path=\"res://scripts/player.gd\" id=\"1\"]\n\n";
		scene += "[node name=\"Main\" type=\"Node2D\"]\n\n";
		scene += "[node name=\"Player\" type=\"CharacterBody2D\" parent=\".\"]\n";
		scene += "script = ExtResource(\"1\")\n\n";
		scene += "[node name=\"CollisionShape2D\" type=\"CollisionShape2D\" parent=\"Player\"]\n";

		_write_file(project_path.path_join("scenes/main.tscn"), scene);
		_write_file(project_path.path_join("scripts/player.gd"), _make_2d_platformer_player_script());
		created_files.push_back("scenes/main.tscn");
		created_files.push_back("scripts/player.gd");

	} else if (template_name == "3d_fps") {
		main_scene = "res://scenes/main.tscn";

		String scene = "[gd_scene load_steps=2 format=3]\n\n";
		scene += "[ext_resource type=\"Script\" path=\"res://scripts/player.gd\" id=\"1\"]\n\n";
		scene += "[node name=\"Main\" type=\"Node3D\"]\n\n";
		scene += "[node name=\"Player\" type=\"CharacterBody3D\" parent=\".\"]\n";
		scene += "script = ExtResource(\"1\")\n\n";
		scene += "[node name=\"Camera3D\" type=\"Camera3D\" parent=\"Player\"]\n";
		scene += "transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1.6, 0)\n\n";
		scene += "[node name=\"CollisionShape3D\" type=\"CollisionShape3D\" parent=\"Player\"]\n\n";
		scene += "[node name=\"DirectionalLight3D\" type=\"DirectionalLight3D\" parent=\".\"]\n";
		scene += "transform = Transform3D(1, 0, 0, 0, 0.707, 0.707, 0, -0.707, 0.707, 0, 10, 0)\n";

		_write_file(project_path.path_join("scenes/main.tscn"), scene);
		_write_file(project_path.path_join("scripts/player.gd"), _make_3d_fps_player_script());
		created_files.push_back("scenes/main.tscn");
		created_files.push_back("scripts/player.gd");

	} else if (template_name == "3d_third_person") {
		main_scene = "res://scenes/main.tscn";

		String scene = "[gd_scene load_steps=2 format=3]\n\n";
		scene += "[ext_resource type=\"Script\" path=\"res://scripts/player.gd\" id=\"1\"]\n\n";
		scene += "[node name=\"Main\" type=\"Node3D\"]\n\n";
		scene += "[node name=\"Player\" type=\"CharacterBody3D\" parent=\".\"]\n";
		scene += "script = ExtResource(\"1\")\n\n";
		scene += "[node name=\"CollisionShape3D\" type=\"CollisionShape3D\" parent=\"Player\"]\n\n";
		scene += "[node name=\"CameraPivot\" type=\"Node3D\" parent=\".\"]\n\n";
		scene += "[node name=\"Camera3D\" type=\"Camera3D\" parent=\"CameraPivot\"]\n";
		scene += "transform = Transform3D(1, 0, 0, 0, 0.9, 0.4, 0, -0.4, 0.9, 0, 3, 5)\n\n";
		scene += "[node name=\"DirectionalLight3D\" type=\"DirectionalLight3D\" parent=\".\"]\n";
		scene += "transform = Transform3D(1, 0, 0, 0, 0.707, 0.707, 0, -0.707, 0.707, 0, 10, 0)\n";

		_write_file(project_path.path_join("scenes/main.tscn"), scene);
		_write_file(project_path.path_join("scripts/player.gd"), _make_3d_third_person_player_script());
		created_files.push_back("scenes/main.tscn");
		created_files.push_back("scripts/player.gd");

	} else if (template_name == "ui_app") {
		main_scene = "res://scenes/main.tscn";

		String scene = "[gd_scene load_steps=2 format=3]\n\n";
		scene += "[ext_resource type=\"Script\" path=\"res://scripts/main.gd\" id=\"1\"]\n\n";
		scene += "[node name=\"Main\" type=\"Control\"]\n";
		scene += "layout_mode = 3\n";
		scene += "anchors_preset = 15\n";
		scene += "anchor_right = 1.0\n";
		scene += "anchor_bottom = 1.0\n";
		scene += "script = ExtResource(\"1\")\n\n";
		scene += "[node name=\"VBoxContainer\" type=\"VBoxContainer\" parent=\".\"]\n";
		scene += "layout_mode = 1\n";
		scene += "anchors_preset = 15\n";
		scene += "anchor_right = 1.0\n";
		scene += "anchor_bottom = 1.0\n\n";
		scene += "[node name=\"Label\" type=\"Label\" parent=\"VBoxContainer\"]\n";
		scene += "layout_mode = 2\n";
		scene += "text = \"Hello World!\"\n\n";
		scene += "[node name=\"Button\" type=\"Button\" parent=\"VBoxContainer\"]\n";
		scene += "layout_mode = 2\n";
		scene += "text = \"Click Me\"\n\n";
		scene += "[connection signal=\"pressed\" from=\"VBoxContainer/Button\" to=\".\" method=\"_on_button_pressed\"]\n";

		_write_file(project_path.path_join("scenes/main.tscn"), scene);
		_write_file(project_path.path_join("scripts/main.gd"), _make_ui_app_main_script());
		created_files.push_back("scenes/main.tscn");
		created_files.push_back("scripts/main.gd");

	} else if (template_name == "empty_2d") {
		main_scene = "res://scenes/main.tscn";

		String scene = "[gd_scene load_steps=2 format=3]\n\n";
		scene += "[ext_resource type=\"Script\" path=\"res://scripts/main.gd\" id=\"1\"]\n\n";
		scene += "[node name=\"Main\" type=\"Node2D\"]\n";
		scene += "script = ExtResource(\"1\")\n";

		_write_file(project_path.path_join("scenes/main.tscn"), scene);
		_write_file(project_path.path_join("scripts/main.gd"), _make_empty_2d_script());
		created_files.push_back("scenes/main.tscn");
		created_files.push_back("scripts/main.gd");

	} else if (template_name == "empty_3d") {
		main_scene = "res://scenes/main.tscn";

		String scene = "[gd_scene load_steps=2 format=3]\n\n";
		scene += "[ext_resource type=\"Script\" path=\"res://scripts/main.gd\" id=\"1\"]\n\n";
		scene += "[node name=\"Main\" type=\"Node3D\"]\n";
		scene += "script = ExtResource(\"1\")\n";

		_write_file(project_path.path_join("scenes/main.tscn"), scene);
		_write_file(project_path.path_join("scripts/main.gd"), _make_empty_3d_script());
		created_files.push_back("scenes/main.tscn");
		created_files.push_back("scripts/main.gd");
	}

	// Write project.godot.
	_write_file(project_path.path_join("project.godot"), _make_project_godot(project_name, main_scene));
	created_files.push_back("project.godot");

	Dictionary result = mcp_success();
	result["template"] = template_name;
	result["project_path"] = project_path;
	result["created_files"] = created_files;
	result["main_scene"] = main_scene;
	return result;
}

// ---------------------------------------------------------------------------
// list_templates
// ---------------------------------------------------------------------------

static Dictionary _tool_list_templates(const Dictionary &p_args) {
	Array templates;

	auto add_template = [&](const String &p_name, const String &p_desc, const String &p_category) {
		Dictionary t;
		t["name"] = p_name;
		t["description"] = p_desc;
		t["category"] = p_category;
		templates.push_back(t);
	};

	add_template("2d_platformer", "2D platformer with CharacterBody2D player, gravity, and basic movement", "2D");
	add_template("3d_fps", "First-person 3D game with mouse look, WASD movement, and jumping", "3D");
	add_template("3d_third_person", "Third-person 3D game with camera follow and character movement", "3D");
	add_template("ui_app", "UI application with Control layout, label, and button", "UI");
	add_template("empty_2d", "Minimal 2D project with Node2D root and empty script", "2D");
	add_template("empty_3d", "Minimal 3D project with Node3D root and empty script", "3D");

	Dictionary result = mcp_success();
	result["templates"] = templates;
	result["count"] = templates.size();
	return result;
}

// ---------------------------------------------------------------------------
// Registration
// ---------------------------------------------------------------------------

void mcp_register_templates_tools(MCPServer *p_server) {
	// create_project_template
	{
		Array template_names;
		template_names.push_back("2d_platformer");
		template_names.push_back("3d_fps");
		template_names.push_back("3d_third_person");
		template_names.push_back("ui_app");
		template_names.push_back("empty_2d");
		template_names.push_back("empty_3d");

		Dictionary schema = MCPSchema::object()
									.prop_enum("template_name", template_names, "Template to use for project generation", true)
									.prop("project_path", "string", "Directory path where the project will be created", true)
									.prop_object("config", "Optional configuration (project_name, etc.)")
									.build();

		p_server->register_tool("create_project_template",
				"Create a new Godot project from a built-in template with scenes, scripts, and folder structure",
				schema, callable_mp_static(_tool_create_project_template));
	}

	// list_templates
	{
		Dictionary schema = MCPSchema::object()
									.build();

		p_server->register_tool("list_templates",
				"List all available project templates with descriptions",
				schema, callable_mp_static(_tool_list_templates));
	}
}
