# Godot AI Fork — MCP Server Complete Reference

> 115 MCP Tools + 3 Resources + 5 Prompts + 4 Events + 14 CLI Commands

---

## MCP Tools (115)

### 1. Core Tools (23) — `tools/mcp_tools_core.cpp`

| # | Tool Name | Description |
|---|-----------|-------------|
| 1 | `ping` | Health check / connectivity test |
| 2 | `get_engine_info` | Get Godot engine version and capabilities |
| 3 | `get_project_structure` | List project directory structure |
| 4 | `get_project_settings` | Read project settings |
| 5 | `set_project_setting` | Modify a project setting |
| 6 | `get_scene_tree` | Get the current scene tree hierarchy |
| 7 | `create_node` | Create a new node in the scene tree |
| 8 | `delete_node` | Remove a node from the scene tree |
| 9 | `modify_node` | Modify node properties |
| 10 | `save_scene` | Save the current scene to disk |
| 11 | `load_scene` | Load a scene file into the editor |
| 12 | `create_script` | Create a new GDScript file |
| 13 | `edit_script` | Edit an existing script's content |
| 14 | `validate_script` | Validate a GDScript file for errors |
| 15 | `get_script_symbols` | Get symbols (classes, functions, variables) from a script |
| 16 | `attach_script` | Attach a script to a node |
| 17 | `run_project` | Run the project in the editor |
| 18 | `stop_project` | Stop the running project |
| 19 | `run_scene` | Run a specific scene |
| 20 | `get_debug_output` | Get debug/console output |
| 21 | `get_errors` | Get current error list |
| 22 | `get_editor_state` | Get editor UI state information |
| 23 | `take_screenshot` | Take a screenshot of the editor viewport |

### 2. Script Editing Tools (4) — `tools/mcp_tools_script_editing.cpp`

| # | Tool Name | Description |
|---|-----------|-------------|
| 24 | `patch_script` | Apply a diff/patch to a script file |
| 25 | `insert_at_line` | Insert text at a specific line number |
| 26 | `delete_lines` | Delete a range of lines from a script |
| 27 | `get_script_content` | Get the full content of a script file |

### 3. 3D Scene Tools (5) — `tools/mcp_tools_3d_scene.cpp`

| # | Tool Name | Description |
|---|-----------|-------------|
| 28 | `create_mesh_instance` | Create a MeshInstance3D node |
| 29 | `create_camera_3d` | Create a Camera3D node |
| 30 | `create_light_3d` | Create a light node (OmniLight3D, SpotLight3D, DirectionalLight3D) |
| 31 | `create_environment` | Create a WorldEnvironment with settings |
| 32 | `create_csg_shape` | Create a CSG shape (box, sphere, cylinder, torus, polygon) |

### 4. Materials Tools (4) — `tools/mcp_tools_materials.cpp`

| # | Tool Name | Description |
|---|-----------|-------------|
| 33 | `create_standard_material` | Create a StandardMaterial3D resource |
| 34 | `apply_material` | Apply a material to a MeshInstance3D |
| 35 | `create_shader_material` | Create a ShaderMaterial with custom shader code |
| 36 | `get_material_properties` | Get all properties of a material |

### 5. Mesh Generation Tools (2) — `tools/mcp_tools_mesh_gen.cpp`

| # | Tool Name | Description |
|---|-----------|-------------|
| 37 | `create_primitive_mesh` | Create a primitive mesh (box, sphere, capsule, cylinder, plane, prism, torus) |
| 38 | `get_mesh_info` | Get mesh information (vertices, faces, AABB) |

### 6. 3D Transform Tools (5) — `tools/mcp_tools_3d_transform.cpp`

| # | Tool Name | Description |
|---|-----------|-------------|
| 39 | `set_transform` | Set a node's position, rotation, and scale |
| 40 | `translate_node` | Translate a node by an offset |
| 41 | `rotate_node` | Rotate a node by euler angles |
| 42 | `look_at_target` | Make a node look at a target position |
| 43 | `get_transform` | Get a node's current transform |

### 7. 3D Physics Tools (7) — `tools/mcp_tools_3d_physics.cpp`

| # | Tool Name | Description |
|---|-----------|-------------|
| 44 | `create_static_body` | Create a StaticBody3D with collision shape |
| 45 | `create_rigid_body` | Create a RigidBody3D with physics properties |
| 46 | `create_character_body_3d` | Create a CharacterBody3D for player/NPC |
| 47 | `create_collision_shape` | Create a CollisionShape3D |
| 48 | `create_area_3d` | Create an Area3D for trigger zones |
| 49 | `raycast_query` | Perform a physics raycast query |
| 50 | `get_physics_state` | Get physics body state information |

### 8. Navigation Tools (5) — `tools/mcp_tools_navigation.cpp`

| # | Tool Name | Description |
|---|-----------|-------------|
| 51 | `create_navigation_region` | Create a NavigationRegion3D |
| 52 | `create_navigation_agent` | Create a NavigationAgent3D |
| 53 | `bake_navigation_mesh` | Bake the navigation mesh |
| 54 | `set_navigation_target` | Set navigation target for an agent |
| 55 | `get_navigation_path` | Get the computed navigation path |

### 9. Particles Tools (3) — `tools/mcp_tools_particles.cpp`

| # | Tool Name | Description |
|---|-----------|-------------|
| 56 | `create_gpu_particles_3d` | Create a GPUParticles3D node |
| 57 | `create_cpu_particles_3d` | Create a CPUParticles3D node |
| 58 | `configure_particle_material` | Configure particle process material |

### 10. Terrain/Environment Tools (4) — `tools/mcp_tools_terrain.cpp`

| # | Tool Name | Description |
|---|-----------|-------------|
| 59 | `setup_3d_environment` | Setup complete 3D environment (presets: outdoor_day, outdoor_night, indoor, space) |
| 60 | `create_directional_light` | Create a directional light with shadow |
| 61 | `create_sky` | Create a sky with procedural or panorama texture |
| 62 | `create_fog` | Create volumetric fog settings |

### 11. Runtime Tools (5) — `tools/mcp_tools_runtime.cpp`

| # | Tool Name | Description |
|---|-----------|-------------|
| 63 | `get_node_runtime_state` | Get a node's runtime state and properties |
| 64 | `call_node_method` | Call a method on a node at runtime |
| 65 | `set_runtime_property` | Set a property on a node at runtime |
| 66 | `get_performance_metrics` | Get engine performance metrics (FPS, memory, draw calls) |
| 67 | `get_physics_state_3d` | Get 3D physics world state |

### 12. Screenshot/Visual Tools (2) — `tools/mcp_tools_screenshot.cpp`

| # | Tool Name | Description |
|---|-----------|-------------|
| 68 | `take_viewport_screenshot` | Take a viewport screenshot (returns base64 PNG) |
| 69 | `get_node_screen_rect` | Get a node's screen-space bounding rect |

### 13. Scene Snapshot Tools (4) — `tools/mcp_tools_snapshot.cpp`

| # | Tool Name | Description |
|---|-----------|-------------|
| 70 | `snapshot_scene` | Save a snapshot of the current scene state |
| 71 | `list_snapshots` | List all saved snapshots |
| 72 | `restore_snapshot` | Restore a previously saved snapshot |
| 73 | `delete_snapshot` | Delete a saved snapshot |

### 14. Batch Tools (1) — `tools/mcp_tools_batch.cpp`

| # | Tool Name | Description |
|---|-----------|-------------|
| 74 | `batch_execute` | Execute multiple tool calls atomically with rollback support |

### 15. Event Tools (3) — `tools/mcp_tools_events.cpp`

| # | Tool Name | Description |
|---|-----------|-------------|
| 75 | `subscribe` | Subscribe to engine events (scene_changed, node_added, script_error, etc.) |
| 76 | `unsubscribe` | Unsubscribe from events |
| 77 | `list_subscriptions` | List active event subscriptions |

### 16. Animation Tools (7) — `tools/mcp_tools_animation.cpp`

| # | Tool Name | Description |
|---|-----------|-------------|
| 78 | `create_animation_player` | Create an AnimationPlayer node |
| 79 | `create_animation` | Create a new Animation resource |
| 80 | `add_animation_track` | Add a track to an animation |
| 81 | `play_animation` | Play an animation |
| 82 | `stop_animation` | Stop an animation |
| 83 | `get_animation_list` | List all animations in an AnimationPlayer |
| 84 | `create_animation_tree` | Create an AnimationTree with blend tree |

### 17. Assets Tools (5) — `tools/mcp_tools_assets.cpp`

| # | Tool Name | Description |
|---|-----------|-------------|
| 85 | `create_resource` | Create a new resource file |
| 86 | `list_resources` | List resources in a directory |
| 87 | `get_resource_info` | Get detailed resource info |
| 88 | `duplicate_resource` | Duplicate a resource |
| 89 | `generate_placeholder_texture` | Generate a solid-color placeholder texture |

### 18. Linter Tools (3) — `tools/mcp_tools_linter.cpp`

| # | Tool Name | Description |
|---|-----------|-------------|
| 90 | `lint_script` | Lint a single GDScript file |
| 91 | `lint_project` | Lint all GDScript files in the project |
| 92 | `get_lint_rules` | Get available lint rules and their descriptions |

### 19. Code Generation Tools (5) — `tools/mcp_tools_codegen.cpp`

| # | Tool Name | Description |
|---|-----------|-------------|
| 93 | `generate_script_template` | Generate a GDScript file from template |
| 94 | `get_available_signals` | Get all signals available for a class |
| 95 | `get_class_properties` | Get all properties of a class |
| 96 | `get_class_methods` | Get all methods of a class |
| 97 | `get_class_hierarchy` | Get the inheritance chain of a class |

### 20. API Documentation Tools (2) — `tools/mcp_tools_api_docs.cpp`

| # | Tool Name | Description |
|---|-----------|-------------|
| 98 | `dump_api_json` | Export complete engine API as JSON |
| 99 | `get_class_doc` | Get documentation for a specific class |

### 21. TileMap Tools (5) — `tools/mcp_tools_tilemap.cpp`

| # | Tool Name | Description |
|---|-----------|-------------|
| 100 | `create_tilemap` | Create a TileMapLayer node |
| 101 | `set_tile` | Set a tile at a specific cell position |
| 102 | `fill_rect` | Fill a rectangle with tiles |
| 103 | `clear_tilemap` | Clear all tiles |
| 104 | `get_tilemap_info` | Get tilemap information |

### 22. Project Templates Tools (2) — `tools/mcp_tools_templates.cpp`

| # | Tool Name | Description |
|---|-----------|-------------|
| 105 | `create_project_template` | Create a project from template (2d_platformer, 3d_fps, 3d_third_person, top_down, puzzle, empty) |
| 106 | `list_templates` | List available project templates |

### 23. Hot Reload Tools (6) — `tools/mcp_tools_hot_reload.cpp`

| # | Tool Name | Description |
|---|-----------|-------------|
| 107 | `trigger_rescan` | Force editor filesystem to rescan for file changes immediately |
| 108 | `reload_script` | Reload a specific script from disk with state preservation |
| 109 | `reload_scene` | Reload the current or a specific scene from disk |
| 110 | `get_modified_files` | List files loaded in cache (filter by scripts/scenes/resources) |
| 111 | `reload_all_scripts` | Reload all currently open scripts from disk |
| 112 | `set_auto_reload` | Configure auto-reload behavior for AI workflow |

### 24. Testing Tools (2) — `tools/mcp_tools_testing.cpp`

| # | Tool Name | Description |
|---|-----------|-------------|
| 113 | `run_tests` | Run GDScript tests and return results as JSON |
| 114 | `discover_tests` | Discover all test files in the test directory |

---

## MCP Events (4)

Events are pushed to MCP subscribers when editor state changes.

| # | Event Name | Description | Data Fields |
|---|------------|-------------|-------------|
| 1 | `filesystem_changed` | Editor filesystem detected changes | `timestamp` |
| 2 | `resources_reloaded` | Resources were reloaded from disk | `paths`, `count`, `timestamp` |
| 3 | `scene_changed` | Active scene changed in editor | `scene_path`, `scene_root`, `timestamp` |
| 4 | `scene_saved` | Scene was saved to disk | `path`, `timestamp` |

---

## MCP Resources (3)

| # | URI | Name | Description |
|---|-----|------|-------------|
| 1 | `godot://scene/current` | Current Scene Tree | JSON representation of the active scene tree |
| 2 | `godot://project/settings` | Project Settings | Current project configuration |
| 3 | `godot://logs/recent` | Recent Logs | Recent engine log messages |

---

## MCP Prompts (5)

| # | Prompt Name | Description | Arguments |
|---|-------------|-------------|-----------|
| 1 | `debug_scene` | Analyze the current scene for issues and suggest fixes | — |
| 2 | `fix_script_errors` | Find and fix all GDScript errors in the project | — |
| 3 | `optimize_scene` | Analyze and optimize the current scene for performance | — |
| 4 | `review_project` | Perform a comprehensive project health check | — |
| 5 | `create_3d_game` | Step-by-step guide to create a 3D game using MCP tools | `game_type` (optional) |

---

## CLI Commands (12)

Direct command-line invocation for AI tools. All output structured JSON to stdout.

```
godot --headless --project <path> <command> [args]
```

| # | Command | Arguments | Description |
|---|---------|-----------|-------------|
| 1 | `--validate-scripts` | — | Validate all GDScript files, report errors as JSON |
| 2 | `--lint-gdscript` | — | Lint all GDScript files for style issues |
| 3 | `--dump-api-json` | `<output_path>` | Export full engine API documentation as JSON |
| 4 | `--project-info` | — | Output project metadata (name, version, features) |
| 5 | `--list-scenes` | — | List all .tscn/.scn scene files |
| 6 | `--list-scripts` | — | List all .gd script files |
| 7 | `--list-resources` | — | List all resource files (.tres, .res, images, audio, etc.) |
| 8 | `--scene-info` | `<scene_path>` | Detailed scene info (nodes, scripts, dependencies) |
| 9 | `--script-info` | `<script_path>` | Detailed script info (classes, functions, signals, variables) |
| 10 | `--export-scene-json` | `<source.tscn> <target.json>` | Convert .tscn scene to JSON format |
| 11 | `--import-scene-json` | `<source.json> <target.tscn>` | Convert JSON back to .tscn format |
| 12 | `--query` | `<expression>` | Query engine data |
| 13 | `--run-tests` | `[directory]` | Run GDScript tests (default: res://tests), output JSON |
| 14 | `--test-verbose` | — | Enable verbose test output (use with --run-tests) |

### Query Expressions

The `--query` command supports the following expressions:

| Expression | Description |
|------------|-------------|
| `get_all_nodes_of_type(Type)` | Find all nodes of a given type in the current scene |
| `find_scripts_using(term)` | Find scripts that reference a search term |
| `find_class(ClassName)` | Get ClassDB information about a class |
| `list_node_types` | List all available node types |
| `list_resource_types` | List all available resource types |

### CLI Usage Examples

```bash
# Validate all scripts in a project
godot --headless --project /my/game --validate-scripts

# Get project overview
godot --headless --project /my/game --project-info

# List all scenes
godot --headless --project /my/game --list-scenes

# Inspect a specific scene
godot --headless --project /my/game --scene-info res://levels/main.tscn

# Find all Camera3D nodes
godot --headless --project /my/game --query "get_all_nodes_of_type(Camera3D)"

# Export scene to JSON for AI editing
godot --headless --project /my/game --export-scene-json res://main.tscn /tmp/main.json

# Lint code quality
godot --headless --project /my/game --lint-gdscript

# Export full API docs
godot --headless --project /my/game --dump-api-json /tmp/godot_api.json

# Run GDScript tests
godot --headless --project /my/game --run-tests

# Run tests with verbose output
godot --headless --project /my/game --run-tests --test-verbose

# Run tests in a specific directory
godot --headless --project /my/game --run-tests res://unit_tests
```

---

## Engine Flags

| Flag | Description |
|------|-------------|
| `--structured-output` | Replace standard logger with JSON Lines structured output |
| `--mcp-stdio` | Start MCP server in stdio mode (for AI tool integration) |
| `--mcp-port <port>` | Start MCP server on WebSocket port (default: 6550) |

---

## Summary

| Category | Count |
|----------|-------|
| MCP Tools | 115 |
| MCP Events | 4 |
| MCP Resources | 3 |
| MCP Prompts | 5 |
| CLI Commands | 14 |
| Query Expressions | 5 |
| Engine Flags | 3 |
| **Total Capabilities** | **146** |
