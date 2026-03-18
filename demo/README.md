# Dodot Demo Project

A minimal 3D game project demonstrating AI-assisted development with dodot.

## What's Included

- `scenes/main.tscn` — 3D scene with floor, sky, lighting, and player
- `scripts/player.gd` — FPS player controller with health system (strict typing, exports, signals)
- `scripts/collectible.gd` — Pickup item with bob/spin animation
- `tests/test_player.gd` — 6 unit tests for player health system
- `tests/test_collectible.gd` — 2 unit tests for collectible

## Try It

```bash
GODOT=/path/to/dodot/bin/godot.linuxbsd.editor.x86_64

# Project overview
$GODOT --headless --path . --project-info

# Validate all scripts
$GODOT --headless --path . --validate-scripts

# Lint for style issues
$GODOT --headless --path . --lint-gdscript

# Run unit tests
$GODOT --headless --path . --run-tests

# Inspect main scene
$GODOT --headless --path . --scene-info res://scenes/main.tscn

# Export scene as JSON
$GODOT --headless --path . --export-scene-json res://scenes/main.tscn /tmp/main.json

# Query a class
$GODOT --headless --path . --query "find_class(CharacterBody3D)"

# Run the game (requires display)
$GODOT --path .
```
