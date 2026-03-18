extends Area3D

## A collectible item that heals the player on contact.

signal collected(item_name: String)

@export var heal_amount: int = 25
@export var bob_speed: float = 2.0
@export var bob_height: float = 0.3
@export var spin_speed: float = 3.0

var _start_y: float = 0.0
var _time: float = 0.0

func _ready() -> void:
	_start_y = position.y
	body_entered.connect(_on_body_entered)

func _process(delta: float) -> void:
	_time += delta
	position.y = _start_y + sin(_time * bob_speed) * bob_height
	rotate_y(spin_speed * delta)

func _on_body_entered(body: Node3D) -> void:
	if body.has_method("heal"):
		body.heal(heal_amount)
		collected.emit(name)
		queue_free()
