extends Node

## Tests for player health system.

func test_initial_health():
	var player := CharacterBody3D.new()
	var script := load("res://scripts/player.gd")
	player.set_script(script)
	if player.health != 100:
		return {"status": "failed", "message": "Expected 100, got " + str(player.health)}
	player.free()
	return true

func test_take_damage():
	var player := CharacterBody3D.new()
	var script := load("res://scripts/player.gd")
	player.set_script(script)
	player.take_damage(25)
	if player.health != 75:
		return {"status": "failed", "message": "Expected 75, got " + str(player.health)}
	player.free()
	return true

func test_take_lethal_damage():
	var player := CharacterBody3D.new()
	var script := load("res://scripts/player.gd")
	player.set_script(script)
	player.take_damage(200)
	if player.health != 0:
		return {"status": "failed", "message": "Health should be 0, got " + str(player.health)}
	if player.is_alive():
		return {"status": "failed", "message": "Player should not be alive"}
	player.free()
	return true

func test_heal():
	var player := CharacterBody3D.new()
	var script := load("res://scripts/player.gd")
	player.set_script(script)
	player.take_damage(50)
	player.heal(20)
	if player.health != 70:
		return {"status": "failed", "message": "Expected 70, got " + str(player.health)}
	player.free()
	return true

func test_heal_does_not_exceed_max():
	var player := CharacterBody3D.new()
	var script := load("res://scripts/player.gd")
	player.set_script(script)
	player.heal(50)
	if player.health != 100:
		return {"status": "failed", "message": "Expected 100, got " + str(player.health)}
	player.free()
	return true

func test_health_percent():
	var player := CharacterBody3D.new()
	var script := load("res://scripts/player.gd")
	player.set_script(script)
	player.take_damage(50)
	var pct: float = player.get_health_percent()
	if pct < 49.9 or pct > 50.1:
		return {"status": "failed", "message": "Expected ~50.0, got " + str(pct)}
	player.free()
	return true
