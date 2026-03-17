# Godot Project — Claude Code Configuration

> Copy this file to your Godot project root as `CLAUDE.md` for optimal AI-assisted development.

## Engine

- Godot version: 4.x-ai-fork (dodot)
- MCP Server: localhost:6550 (auto-starts with editor)
- Language: GDScript (strict typing recommended)

## Coding Standards

- All variables must have type annotations: `var speed: float = 300.0`
- All functions must have return types: `func get_health() -> int:`
- Signals use snake_case: `signal health_changed(new_health: int)`
- Prefer %UniqueNode over hardcoded NodePath
- Use @export for configurable parameters in scenes
- Test files go in `res://tests/` with `test_` prefix

## MCP Workflow

1. Connect to MCP server at `localhost:6550` or use `--mcp-stdio`
2. Edit `.gd` files — hot reload triggers automatically
3. Use `get_errors()` to check for script errors
4. Use `run_project()` / `take_screenshot()` to test visually
5. Use `--run-tests` for automated test verification

## CLI Commands (Headless)

```bash
# Validate all scripts
godot --headless --path . --validate-scripts

# Static analysis
godot --headless --path . --lint-gdscript

# Run tests
godot --headless --path . --run-tests

# Project overview
godot --headless --path . --project-info

# List all scenes/scripts
godot --headless --path . --list-scenes
godot --headless --path . --list-scripts

# Scene inspection
godot --headless --path . --scene-info res://main.tscn

# Export scene as JSON (AI-readable)
godot --headless --path . --export-scene-json res://main.tscn /tmp/main.json

# Query engine data
godot --headless --path . --query "find_class(CharacterBody3D)"

# Export full API documentation
godot --headless --path . --dump-api-json /tmp/api.json
```

## JSON Scene Format

This fork supports `.scene.json` as a native scene format. AI tools can directly read/write scenes in JSON:

```gdscript
# Load JSON scene
var scene = load("res://level.scene.json")

# Save as JSON scene
ResourceSaver.save(packed_scene, "res://level.scene.json")
```

## Testing with GDTest

Create test files in `res://tests/` with `test_` prefix:

```gdscript
# res://tests/test_player.gd
extends Node

func test_player_health():
    var player = Player.new()
    player.health = 100
    player.take_damage(25)
    if player.health != 75:
        return {"status": "failed", "message": "Expected 75, got " + str(player.health)}
    return true

func test_player_name():
    var player = Player.new()
    if player.name.is_empty():
        return {"status": "failed", "message": "Name should not be empty"}
    return true
```

Run with: `godot --headless --path . --run-tests`

## Project Structure

```
res://
  scenes/     — Scene files (.tscn or .scene.json)
  scripts/    — GDScript files
  resources/  — Custom resources (.tres)
  tests/      — Unit tests (test_*.gd)
  assets/     — Art, audio, fonts
```

## MCP Tools Reference

See the full tool reference at: https://github.com/SilvianYiu/dodot/blob/master/modules/mcp_server/REFERENCE.md
