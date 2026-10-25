# exercises tool, rpc, setget, constructors, inner classes, match patterns
extends Node

class_name Custom, "res://icon.svg"

tool

# constants
const MAX_SPEED: int = 100
const GREETING = "hello \u0041 world"

# signals
signal died
signal hit(damage, source)

# enums
enum {UNIT_NEUTRAL, UNIT_ENEMY}
enum State {IDLE, RUNNING = 1, JUMPING}

# exported / onready vars
export(int) var health = 100
export(String, "a", "b") var mode = "a"
onready var sprite = $Sprite
var typed: int
var inferred := 3.14
var with_setget = 0 setget set_with, get_with

static func helper(a, b = 1):
	return a + b

func _init(x).(x, 1):
	pass

func compute(a, b):
	var result = a + b * 2
	if result > 10:
		return result
	elif result == 10:
		pass
	else:
		return -result

	for i in range(3):
		if i is int:
			print(i)

	var f = 1.5e10
	var g = .5
	var shorthand_dict = {a = 1, b = 2}

	var u1 = ~5
	var u2 = -5
	var u3 = +5
	var u4 = !true
	var u5 = not true

	assert(result >= 0, "must be non-negative")

	var node = $"../Other"
	var path = $Sprite/Anim

	var x = result as float
	var t = 1 if result > 0 else -1

	match result:
		0:
			pass
		1, 2:
			pass
		var n:
			print(n)
		_:
			pass
		[a, b..]:
			pass
		{"key": value ..}:
			pass

	yield(self, "done")
	preload("res://icon.png")

	return .compute(a, b)

class Inner extends Node:
	var inner_var = 1

class Inner2:
	extends Node2D
	var another = 2
