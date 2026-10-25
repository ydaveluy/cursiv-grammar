package coverage

import alias_fmt "core:fmt"

multi_ptr :: proc(p: [^]int) -> int {
	return p[0]
}

slices_and_dynamic :: proc() {
	xs: [dynamic; 4]int
	ys := []int{1, 2, 3, 4, 5}
	a := ys[1:3]
	b := ys[1:3:4]
	_ = xs
	_ = a
	_ = b
}

row_major_matrix :: proc() {
	m: #row_major matrix[2, 2]f32
	_ = m
}

reverse_loop :: proc() {
	xs := [3]int{1, 2, 3}
	#reverse for x in xs {
		_ = x
	}
}

TIGHT_OK :: true

Tight :: struct #min_field_align(2) #simple #all_or_none where TIGHT_OK {
	a: i8,
	b: i32,
}

inferred_soa :: proc() {
	s: #soa[?]int
	_ = s
}

loaded :: #load("data.bin")

defined_check :: proc() {
	x := 1
	ok := #defined(x)
	_ = ok
}

untyped_pointers :: proc(callback: proc(int) -> int, id: typeid, p: rawptr) {
	_ = callback(1)
	_ = id
	_ = p
}

main :: proc() {
	_ = alias_fmt
	_ = loaded
	untyped_pointers(proc(x: int) -> int { return x }, typeid_of(int), nil)
}
