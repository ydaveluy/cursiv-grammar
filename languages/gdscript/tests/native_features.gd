tool
class_name AuditNative
extends Node

signal changed(value, source,)
export(int, -10, 10) var health := 1
remote var state = 0
const ORDER = 10 - 3 + 1
const TEXT = """first
  # text
last"""
const PATH = @"a/b:c"
const VALUE := 1_000

enum State {IDLE = -2, RUNNING = 2 + 2, DONE}

func sum_values(a: int, b: int = 3) -> int:
	return a + b

func checked(message):
	assert(message != null, str(message))
	var flags = 1
	flags <<= 2
	flags >>= 1
	return flags

func delayed():
	var state = yield()
	return state

func native_child():
	return $Root/"space name"/Child

func select(value):
	match value:
		{1: var x, 2, ..}:
			return x
		[1, ..]:
			return 3
		-2:
			return 4
		_:
			return 0

class Outer:
	class Inner:
		pass

func typed(item: Outer.Inner) -> Outer.Inner:
	return item
