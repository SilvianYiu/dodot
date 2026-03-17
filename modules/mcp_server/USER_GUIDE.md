# Dodot User Guide

> Build Godot games with AI assistance — using Claude Code, Cursor, or any MCP-compatible tool.

---

## Table of Contents

1. [Getting Started](#getting-started)
2. [Connection Methods](#connection-methods)
3. [CLI Commands (Headless)](#cli-commands-headless)
4. [MCP Tools (Editor)](#mcp-tools-editor)
5. [Writing & Running Tests](#writing--running-tests)
6. [JSON Scene Format](#json-scene-format)
7. [GDScript LSP Enhancements](#gdscript-lsp-enhancements)
8. [Structured Output](#structured-output)
9. [Workflow Examples](#workflow-examples)
10. [Claude Code Configuration](#claude-code-configuration)
11. [Troubleshooting](#troubleshooting)

---

## Getting Started

### Prerequisites

- Built dodot binary (see build instructions below)
- A Godot 4.x project (or create one using the editor)
- An MCP-compatible AI tool (Claude Code, Cursor, etc.)

### Building from Source

```bash
# Clone the repository
git clone https://github.com/SilvianYiu/dodot.git
cd dodot

# Build the editor
scons platform=linuxbsd target=editor module_mcp_server_enabled=yes module_gdtest_enabled=yes -j$(nproc)

# Binary output
./bin/godot.linuxbsd.editor.x86_64
```

### Quick Start

```bash
# 1. Create a project
mkdir my_game && cd my_game
echo '[gd_resource type="ProjectSettings"]' > project.godot

# 2. Validate it works
godot --headless --path . --project-info

# 3. Connect your AI tool (see Connection Methods below)
```

---

## Connection Methods

Dodot supports two transport modes for AI tool integration:

### Method 1: stdio (Recommended for Claude Code)

The AI tool launches Godot as a subprocess and communicates via stdin/stdout using the MCP protocol.

```bash
godot --headless --path /path/to/project --mcp-stdio
```

For Claude Code, add this to your MCP server configuration:

```json
{
  "mcpServers": {
    "godot": {
      "command": "/path/to/godot",
      "args": ["--headless", "--path", "/path/to/project", "--mcp-stdio"]
    }
  }
}
```

### Method 2: WebSocket (Recommended for Editor Use)

When the editor is running, the MCP server starts automatically on port 6550.

```bash
# Launch editor (MCP server auto-starts)
godot --path /path/to/project

# Or specify a custom port
godot --path /path/to/project --mcp-port 7000
```

Connect your AI tool to `ws://localhost:6550`.

### Method 3: CLI Only (No Server Needed)

For quick one-off queries, use the CLI commands directly. No MCP connection required.

```bash
godot --headless --path /path/to/project --validate-scripts
```

---

## CLI Commands (Headless)

All CLI commands run headlessly and output structured JSON to stdout. They require no editor or MCP connection.

### Project Overview

```bash
# Get project metadata
godot --headless --path . --project-info

# List all scene files
godot --headless --path . --list-scenes

# List all script files
godot --headless --path . --list-scripts

# List all resource files
godot --headless --path . --list-resources
```

### Script Validation & Linting

```bash
# Check all scripts for errors (type errors, syntax errors)
godot --headless --path . --validate-scripts

# Lint all scripts for style issues (missing types, naming conventions)
godot --headless --path . --lint-gdscript
```

Example output:

```json
{
  "command": "validate-scripts",
  "success": true,
  "summary": { "passed": 12, "failed": 0, "total": 12, "warnings": 0 },
  "results": [
    { "file": "res://player.gd", "status": "passed" },
    { "file": "res://enemy.gd", "status": "passed" }
  ]
}
```

### Scene Inspection

```bash
# Get detailed node tree for a scene
godot --headless --path . --scene-info res://main.tscn

# Export a scene as JSON for AI editing
godot --headless --path . --export-scene-json res://main.tscn /tmp/main.json

# Convert JSON back to .tscn
godot --headless --path . --import-scene-json /tmp/main.json res://main_new.tscn
```

### Script Inspection

```bash
# Get script metadata (classes, functions, signals, variables)
godot --headless --path . --script-info res://player.gd
```

### Engine API

```bash
# Query class information
godot --headless --path . --query "find_class(CharacterBody3D)"

# Find all node types
godot --headless --path . --query "list_node_types"

# Export full API documentation (1000+ classes)
godot --headless --path . --dump-api-json /tmp/api.json
```

### Testing

```bash
# Run all tests
godot --headless --path . --run-tests

# Run with verbose output
godot --headless --path . --run-tests --test-verbose

# Run tests from a specific directory
godot --headless --path . --run-tests res://unit_tests
```

---

## MCP Tools (Editor)

When connected via MCP (stdio or WebSocket), you have access to **115 tools** organized by category. These tools operate on the running editor instance.

### Core Workflow

The most common tool sequence for AI-assisted development:

```
1. get_project_structure    → Understand the project layout
2. get_scene_tree           → See the current scene hierarchy
3. create_script / edit_script → Write or modify code
4. validate_script          → Check for errors
5. run_project              → Test it
6. take_screenshot          → See the result
7. get_errors               → Check for runtime errors
```

### Tool Categories

| Category | Count | Key Tools |
|----------|-------|-----------|
| Core | 23 | `get_scene_tree`, `create_node`, `modify_node`, `create_script`, `edit_script`, `run_project`, `take_screenshot` |
| Script Editing | 4 | `patch_script`, `insert_at_line`, `delete_lines`, `get_script_content` |
| 3D Scene | 5 | `create_mesh_instance`, `create_camera_3d`, `create_light_3d`, `create_csg_shape` |
| Materials | 4 | `create_standard_material`, `apply_material`, `create_shader_material` |
| Mesh Generation | 2 | `create_primitive_mesh`, `get_mesh_info` |
| 3D Transform | 5 | `set_transform`, `translate_node`, `rotate_node`, `look_at_target` |
| 3D Physics | 7 | `create_static_body`, `create_rigid_body`, `create_character_body_3d`, `raycast_query` |
| Navigation | 5 | `create_navigation_region`, `create_navigation_agent`, `bake_navigation_mesh` |
| Particles | 3 | `create_gpu_particles_3d`, `create_cpu_particles_3d`, `configure_particle_material` |
| Environment | 4 | `setup_3d_environment`, `create_directional_light`, `create_sky`, `create_fog` |
| Runtime | 5 | `get_node_runtime_state`, `call_node_method`, `set_runtime_property`, `get_performance_metrics` |
| Screenshot | 2 | `take_viewport_screenshot`, `get_node_screen_rect` |
| Snapshots | 4 | `snapshot_scene`, `list_snapshots`, `restore_snapshot` |
| Batch | 1 | `batch_execute` (run multiple tools atomically) |
| Events | 3 | `subscribe`, `unsubscribe`, `list_subscriptions` |
| Animation | 7 | `create_animation_player`, `create_animation`, `add_animation_track`, `play_animation` |
| Assets | 5 | `create_resource`, `list_resources`, `duplicate_resource` |
| Linter | 3 | `lint_script`, `lint_project`, `get_lint_rules` |
| Code Generation | 5 | `generate_script_template`, `get_available_signals`, `get_class_properties`, `get_class_methods` |
| API Docs | 2 | `dump_api_json`, `get_class_doc` |
| TileMap | 5 | `create_tilemap`, `set_tile`, `fill_rect`, `clear_tilemap` |
| Templates | 2 | `create_project_template` (2d_platformer, 3d_fps, 3d_third_person, etc.) |
| Hot Reload | 6 | `trigger_rescan`, `reload_script`, `reload_scene`, `reload_all_scripts`, `set_auto_reload` |
| Testing | 2 | `run_tests`, `discover_tests` |

### MCP Resources

Read-only data the AI can access:

| URI | Description |
|-----|-------------|
| `godot://scene/current` | Current scene tree as JSON |
| `godot://project/settings` | Project configuration |
| `godot://logs/recent` | Recent engine log messages |

### MCP Events

Subscribe to real-time editor events:

| Event | Triggered When |
|-------|---------------|
| `filesystem_changed` | Files added, removed, or modified on disk |
| `resources_reloaded` | Resources reloaded from disk |
| `scene_changed` | Active scene changed in editor |
| `scene_saved` | Scene saved to disk |

### MCP Prompts

Pre-built analysis prompts:

| Prompt | Description |
|--------|-------------|
| `debug_scene` | Analyze current scene for issues |
| `fix_script_errors` | Find and fix all script errors |
| `optimize_scene` | Performance optimization suggestions |
| `review_project` | Comprehensive project health check |
| `create_3d_game` | Step-by-step game creation guide |

---

## Writing & Running Tests

Dodot includes **GDTest**, a built-in testing framework for GDScript.

### Writing Tests

Create test files in `res://tests/` with a `test_` prefix:

```gdscript
# res://tests/test_player.gd
extends Node

# Optional lifecycle hooks
func before_all():
    print("Setting up test suite")

func before_each():
    pass  # Runs before each test

func after_each():
    pass  # Runs after each test

func after_all():
    print("Tearing down test suite")

# Test methods must start with test_
func test_player_starts_with_full_health():
    var player = Player.new()
    if player.health != 100:
        return {"status": "failed", "message": "Expected 100, got " + str(player.health)}
    return true

func test_player_takes_damage():
    var player = Player.new()
    player.take_damage(25)
    if player.health != 75:
        return {"status": "failed", "message": "Expected 75, got " + str(player.health)}
    player.free()
    return true

func test_player_cannot_have_negative_health():
    var player = Player.new()
    player.take_damage(200)
    if player.health < 0:
        return {"status": "failed", "message": "Health should not be negative"}
    player.free()
    return true
```

### Test Return Values

| Return Value | Meaning |
|-------------|---------|
| `true` | Test passed |
| `{"status": "failed", "message": "..."}` | Test failed with message |
| `{"status": "skipped", "message": "..."}` | Test skipped |
| (exception thrown) | Test error |

### Running Tests

```bash
# Run all tests in res://tests/
godot --headless --path . --run-tests

# Run with verbose output
godot --headless --path . --run-tests --test-verbose

# Run tests from a specific directory
godot --headless --path . --run-tests res://unit_tests
```

### Via MCP

```json
{"method": "tools/call", "params": {"name": "run_tests", "arguments": {}}}
{"method": "tools/call", "params": {"name": "discover_tests", "arguments": {}}}
```

### Test Output Format

```json
{
  "framework": "GDTest",
  "summary": {
    "total": 5,
    "passed": 4,
    "failed": 1,
    "errors": 0,
    "skipped": 0,
    "duration_ms": 12.5,
    "success": false
  },
  "tests": [
    {
      "test_name": "test_player_takes_damage",
      "suite_name": "test_player",
      "file_path": "res://tests/test_player.gd",
      "status": "passed",
      "message": "",
      "duration_ms": 0.5
    }
  ]
}
```

---

## JSON Scene Format

Dodot natively supports `.scene.json` as a scene file format. AI tools can directly read and write scene files as structured JSON.

### Why JSON Scenes?

- **AI-readable**: No need to parse Godot's `.tscn` text format
- **Diffable**: Clean JSON diffs in version control
- **Round-trip safe**: Load a `.tscn`, save as `.scene.json`, load it back — lossless
- **Type-preserved**: All Godot types (Vector3, Color, Transform3D, etc.) use `_type` tags

### Creating a JSON Scene

```bash
# Convert existing .tscn to .scene.json
godot --headless --path . --export-scene-json res://main.tscn main.scene.json
```

### JSON Scene Structure

```json
{
  "type": "PackedScene",
  "format_version": 1,
  "external_resources": [
    {"id": "ext_1", "type": "Script", "path": "res://player.gd"}
  ],
  "sub_resources": [
    {
      "id": "sub_1",
      "type": "BoxMesh",
      "properties": {
        "size": {"_type": "Vector3", "x": 1.0, "y": 1.0, "z": 1.0}
      }
    }
  ],
  "nodes": [
    {
      "name": "Root",
      "type": "Node3D",
      "parent": ""
    },
    {
      "name": "Player",
      "type": "CharacterBody3D",
      "parent": ".",
      "properties": {
        "transform": {
          "_type": "Transform3D",
          "basis": {
            "rows": [
              {"x": 1, "y": 0, "z": 0},
              {"x": 0, "y": 1, "z": 0},
              {"x": 0, "y": 0, "z": 1}
            ]
          },
          "origin": {"x": 0, "y": 1.5, "z": 0}
        },
        "script": {"_type": "ExtResource", "id": "ext_1"}
      }
    },
    {
      "name": "Mesh",
      "type": "MeshInstance3D",
      "parent": "./Player",
      "properties": {
        "mesh": {"_type": "SubResource", "id": "sub_1"}
      }
    }
  ],
  "connections": [
    {
      "from": "Player",
      "signal": "health_changed",
      "to": ".",
      "method": "_on_player_health_changed",
      "flags": 0
    }
  ]
}
```

### Type Tags

Properties use `_type` to preserve Godot types in JSON:

| `_type` | Example |
|---------|---------|
| `Vector2` | `{"_type": "Vector2", "x": 1.0, "y": 2.0}` |
| `Vector3` | `{"_type": "Vector3", "x": 1.0, "y": 2.0, "z": 3.0}` |
| `Color` | `{"_type": "Color", "r": 1.0, "g": 0.0, "b": 0.0, "a": 1.0}` |
| `Transform3D` | `{"_type": "Transform3D", "basis": {...}, "origin": {...}}` |
| `NodePath` | `{"_type": "NodePath", "path": "../Player"}` |
| `ExtResource` | `{"_type": "ExtResource", "id": "ext_1"}` |
| `SubResource` | `{"_type": "SubResource", "id": "sub_1"}` |

Full list: Vector2, Vector2i, Vector3, Vector3i, Vector4, Vector4i, Rect2, Rect2i, Color, Transform2D, Basis, Transform3D, Quaternion, AABB, Plane, Projection, NodePath, StringName, RID, and all Packed*Array types.

### Using in GDScript

```gdscript
# Load a JSON scene
var scene: PackedScene = load("res://level.scene.json")
var root: Node = scene.instantiate()
add_child(root)

# Save a scene as JSON
var packed := PackedScene.new()
packed.pack(get_tree().current_scene)
ResourceSaver.save(packed, "res://level.scene.json")
```

---

## GDScript LSP Enhancements

The built-in GDScript Language Server has been enhanced with two new capabilities. These work automatically with any LSP-compatible editor (VS Code with Godot extension, Neovim, etc.).

### Inlay Hints

**Parameter name hints** appear at function call sites with 2+ arguments:

```gdscript
# Before: hard to remember parameter order
move_and_slide(velocity, up_direction, stop_on_slope)

# With inlay hints displayed:
move_and_slide(velocity: velocity, up_direction: up_direction, stop_on_slope: true)
```

**Inferred type hints** appear on variables without explicit annotations:

```gdscript
var speed = 300.0        # shows ": float" after "speed"
var name = "Player"      # shows ": String" after "name"
```

### Code Actions

Quick fixes offered by the language server:

| Action | Trigger | Fix |
|--------|---------|-----|
| Prefix unused variable | "Unused variable 'x'" warning | Renames `x` to `_x` |
| Add type annotation | "Missing type annotation" lint | Adds `: Type` based on inference |
| Use `:=` for inference | "Use := for type inference" | Changes `var x = 1` to `var x := 1` |

---

## Structured Output

The `--structured-output` flag replaces Godot's standard logger with JSON Lines output. Every log message becomes a JSON object on stdout.

```bash
godot --headless --path . --structured-output --validate-scripts
```

Output format:

```json
{"type":"info","timestamp":"2025-01-15T10:30:00Z","message":"Godot Engine v4.7.dev"}
{"type":"warning","timestamp":"2025-01-15T10:30:01Z","message":"Script has unused variable"}
{"command":"validate-scripts","success":true,"summary":{"passed":3,"failed":0}}
```

This is useful for AI tools that need to parse engine output programmatically.

---

## Workflow Examples

### Example 1: Create a 3D Game from Scratch (MCP)

```
AI → tools/call: create_project_template {type: "3d_fps"}
AI → tools/call: get_scene_tree {}
AI → tools/call: create_script {path: "res://player.gd", content: "..."}
AI → tools/call: attach_script {node: "Player", script: "res://player.gd"}
AI → tools/call: run_project {}
AI → tools/call: take_screenshot {}
AI → tools/call: get_errors {}
AI → tools/call: edit_script {path: "res://player.gd", content: "...fixed..."}
AI → tools/call: reload_script {path: "res://player.gd"}
```

### Example 2: Debug a Scene (CLI)

```bash
# Step 1: What's in the project?
godot --headless --path . --project-info

# Step 2: Any script errors?
godot --headless --path . --validate-scripts

# Step 3: Code quality issues?
godot --headless --path . --lint-gdscript

# Step 4: Inspect the main scene
godot --headless --path . --scene-info res://main.tscn

# Step 5: Run tests
godot --headless --path . --run-tests
```

### Example 3: AI Edits a Scene via JSON

```bash
# 1. Export scene to JSON
godot --headless --path . --export-scene-json res://level.tscn /tmp/level.json

# 2. AI reads and modifies the JSON file
#    (add nodes, change properties, add connections)

# 3. Import modified JSON back
godot --headless --path . --import-scene-json /tmp/level.json res://level_v2.tscn
```

### Example 4: Continuous Development Loop (MCP Events)

```
AI → tools/call: subscribe {event: "filesystem_changed"}
AI → tools/call: subscribe {event: "scene_saved"}
AI → tools/call: set_auto_reload {enabled: true}

# Now the AI gets notified when files change:
Event ← filesystem_changed {timestamp: "..."}
AI → tools/call: validate_script {path: "res://player.gd"}
AI → tools/call: run_tests {directory: "res://tests"}
```

---

## Claude Code Configuration

Copy `CLAUDE_TEMPLATE.md` from `modules/mcp_server/` to your project root as `CLAUDE.md`:

```bash
cp /path/to/dodot/modules/mcp_server/CLAUDE_TEMPLATE.md /path/to/your/project/CLAUDE.md
```

This gives Claude Code project-specific context about:
- Available CLI commands
- Coding standards (strict typing, signals, exports)
- Testing patterns with GDTest
- MCP workflow steps
- JSON scene format usage

---

## Troubleshooting

### MCP server won't start

- **Check port availability**: `lsof -i :6550` — if occupied, use `--mcp-port <other_port>`
- **Use stdio mode**: `--mcp-stdio` avoids port conflicts entirely
- **Check editor logs**: Look for "MCP Server started" in the Output panel

### CLI commands return nothing

- Ensure `--headless` and `--path` are specified **before** the command flag
- Verify the project has a valid `project.godot` file
- Commands output to stdout; errors go to stderr

### Scripts fail validation but work in editor

- CLI validation runs without editor context. Some autoloads and plugins may not be available.
- Ensure scripts don't depend on editor-only features in their class body.

### JSON scene won't load

- Check `format_version` is `1`
- Verify all `ExtResource` paths exist and are loadable
- Check `_type` tags are spelled correctly (case-sensitive)
- Use `--validate-scripts` to check for script errors referenced by the scene

### Tests aren't discovered

- Test files must be in `res://tests/` (or directory specified with `--run-tests`)
- Filenames must start with `test_` and end with `.gd`
- Test methods must start with `test_`
- The script must be parseable (no syntax errors)

### Hot reload not working

- Call `trigger_rescan` after writing files externally
- Or enable `set_auto_reload {enabled: true}` for automatic detection
- Ensure the editor is running (hot reload requires the editor process)
