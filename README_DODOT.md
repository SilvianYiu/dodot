# Dodot — AI-Friendly Godot Engine Fork

An AI-native fork of [Godot Engine](https://godotengine.org/) that lets Claude Code, Cursor, and other MCP-compatible tools directly control the editor, run tests, validate code, and build games.

Built on Godot 4.x. All additions are modular — no core API changes, easy to rebase on upstream.

## What's Added

| Feature | Description |
|---------|-------------|
| **MCP Server** | 115 tools for scene manipulation, scripting, debugging, animation, physics, and more — accessible via WebSocket or stdio |
| **14 CLI Commands** | Headless project inspection, script validation, linting, scene export, API docs, testing |
| **GDTest** | Built-in GDScript testing framework with JSON output |
| **JSON Scenes** | Native `.scene.json` format — AI-readable, diffable, round-trip lossless |
| **LSP Enhancements** | Inlay hints (parameter names, inferred types) and code actions (quick fixes) |
| **Structured Output** | JSON Lines logger for machine-parseable engine output |
| **Hot Reload** | 6 tools + 4 events for instant AI iteration loops |

## Quick Start

### Build

```bash
git clone https://github.com/SilvianYiu/dodot.git
cd dodot
scons platform=linuxbsd target=editor module_mcp_server_enabled=yes module_gdtest_enabled=yes -j$(nproc)
```

Binary: `bin/godot.linuxbsd.editor.x86_64`

### Connect Claude Code (stdio)

Add to your MCP config:

```json
{
  "mcpServers": {
    "godot": {
      "command": "./bin/godot.linuxbsd.editor.x86_64",
      "args": ["--headless", "--path", "/path/to/project", "--mcp-stdio"]
    }
  }
}
```

### Try the CLI

```bash
GODOT=./bin/godot.linuxbsd.editor.x86_64

# Project overview
$GODOT --headless --path /path/to/project --project-info

# Validate all scripts
$GODOT --headless --path /path/to/project --validate-scripts

# Lint for style issues
$GODOT --headless --path /path/to/project --lint-gdscript

# Run tests
$GODOT --headless --path /path/to/project --run-tests

# Export scene as JSON
$GODOT --headless --path /path/to/project --export-scene-json res://main.tscn /tmp/main.json

# Query engine API
$GODOT --headless --path /path/to/project --query "find_class(CharacterBody3D)"
```

All commands output structured JSON to stdout.

## MCP Tools (115)

When connected via MCP, the AI has access to:

| Category | Tools | Examples |
|----------|-------|---------|
| Core | 23 | `get_scene_tree`, `create_node`, `edit_script`, `run_project`, `take_screenshot` |
| Script Editing | 4 | `patch_script`, `insert_at_line`, `delete_lines` |
| 3D Scene | 5 | `create_mesh_instance`, `create_camera_3d`, `create_light_3d` |
| Materials | 4 | `create_standard_material`, `apply_material`, `create_shader_material` |
| 3D Transform | 5 | `set_transform`, `translate_node`, `rotate_node`, `look_at_target` |
| 3D Physics | 7 | `create_static_body`, `create_rigid_body`, `create_character_body_3d` |
| Navigation | 5 | `create_navigation_region`, `bake_navigation_mesh` |
| Animation | 7 | `create_animation_player`, `create_animation`, `add_animation_track` |
| Particles | 3 | `create_gpu_particles_3d`, `configure_particle_material` |
| Environment | 4 | `setup_3d_environment`, `create_sky`, `create_fog` |
| Runtime | 5 | `call_node_method`, `get_performance_metrics` |
| Hot Reload | 6 | `reload_script`, `reload_scene`, `set_auto_reload` |
| Testing | 2 | `run_tests`, `discover_tests` |
| Linter | 3 | `lint_script`, `lint_project` |
| Code Gen | 5 | `generate_script_template`, `get_class_methods` |
| API Docs | 2 | `dump_api_json`, `get_class_doc` |
| Assets | 5 | `create_resource`, `duplicate_resource` |
| TileMap | 5 | `create_tilemap`, `set_tile`, `fill_rect` |
| Templates | 2 | `create_project_template` (6 presets) |
| Snapshots | 4 | `snapshot_scene`, `restore_snapshot` |
| Batch | 1 | `batch_execute` (atomic multi-tool) |
| Events | 3 | `subscribe`, `unsubscribe` |
| Screenshots | 2 | `take_viewport_screenshot` |

Plus **3 resources**, **5 prompts**, and **4 real-time events**.

Full reference: [modules/mcp_server/REFERENCE.md](modules/mcp_server/REFERENCE.md)

## Testing with GDTest

```gdscript
# res://tests/test_player.gd
extends Node

func test_health_decreases():
    var player = Player.new()
    player.take_damage(25)
    if player.health != 75:
        return {"status": "failed", "message": "Expected 75, got " + str(player.health)}
    return true
```

```bash
$GODOT --headless --path . --run-tests
```

```json
{
  "framework": "GDTest",
  "summary": {"total": 1, "passed": 1, "failed": 0, "duration_ms": 0.5}
}
```

## JSON Scene Format

Native `.scene.json` support — load and save PackedScene as JSON:

```gdscript
var scene = load("res://level.scene.json")
ResourceSaver.save(packed_scene, "res://level.scene.json")
```

Types are preserved via `_type` tags:
```json
{"_type": "Vector3", "x": 1.0, "y": 2.0, "z": 3.0}
{"_type": "Color", "r": 1.0, "g": 0.0, "b": 0.0, "a": 1.0}
{"_type": "Transform3D", "basis": {"rows": [...]}, "origin": {"x": 0, "y": 0, "z": 0}}
```

## For Game Projects

Copy `modules/mcp_server/CLAUDE_TEMPLATE.md` to your game project root as `CLAUDE.md` to give Claude Code full context about available tools, coding standards, and workflows.

## Documentation

- [User Guide](modules/mcp_server/USER_GUIDE.md) — comprehensive how-to
- [Tool Reference](modules/mcp_server/REFERENCE.md) — all 115 tools, events, resources, prompts
- [CLAUDE.md Template](modules/mcp_server/CLAUDE_TEMPLATE.md) — for game projects

## Architecture

All additions are modular:

```
modules/mcp_server/     — MCP protocol, 115 tools (24 tool files), editor plugin, CLI handler
modules/gdtest/         — GDScript testing framework
modules/gdscript/language_server/ — LSP enhancements (inlay hints, code actions)
core/io/structured_logger.h/.cpp  — JSON Lines structured output
scene/resources/resource_format_scene_json.h/.cpp — Native .scene.json format
```

## Building for Different Targets

```bash
# Full editor with all AI features
scons platform=linuxbsd target=editor module_mcp_server_enabled=yes module_gdtest_enabled=yes -j$(nproc)

# Export template (no dev tools)
scons platform=linuxbsd target=template_release module_mcp_server_enabled=no module_gdtest_enabled=no -j$(nproc)

# macOS
scons platform=macos target=editor module_mcp_server_enabled=yes module_gdtest_enabled=yes -j$(nproc)

# Windows (cross-compile or native)
scons platform=windows target=editor module_mcp_server_enabled=yes module_gdtest_enabled=yes -j$(nproc)
```

## License

Same as Godot Engine — [MIT License](LICENSE.txt).
