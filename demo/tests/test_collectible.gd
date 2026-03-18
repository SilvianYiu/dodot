extends Node

## Tests for collectible item behavior.

func test_collectible_has_default_heal():
	var collectible := Area3D.new()
	var script := load("res://scripts/collectible.gd")
	collectible.set_script(script)
	if collectible.heal_amount != 25:
		return {"status": "failed", "message": "Expected 25, got " + str(collectible.heal_amount)}
	collectible.free()
	return true

func test_collectible_custom_heal():
	var collectible := Area3D.new()
	var script := load("res://scripts/collectible.gd")
	collectible.set_script(script)
	collectible.heal_amount = 50
	if collectible.heal_amount != 50:
		return {"status": "failed", "message": "Expected 50, got " + str(collectible.heal_amount)}
	collectible.free()
	return true
