module features

import os
import net.http as net

const (
	max_count = 100
	pi = 3
)

struct Point {
	x int
	y int
pub:
	label string
pub mut:
	tag string
mut:
	hits int
}

interface Shape {
	area() int
}

struct Container {
	shape Shape
}

fn (p Point) sum() int {
	return p.x + p.y
}

fn make_point(x int, y int) Point {
	return Point{x: x, y: y}
}

fn apply(f fn(int) int, v int) int {
	return f(v)
}

fn main() {
	mut count := 0
	names := ['a', 'b', 'c']
	scores := {'a': 1, 'b': 2}
	grid := [3]int{}
	p := Point{x: 1, y: 2}
	q := &p
	count++
	count += 2
	for i := 0; i < 3; i++ {
		count = count + i
	}
	for count < 10 {
		count++
	}
	for n in names {
		println(n)
	}
	if count > 5 {
		println('big')
	} else if count > 0 {
		println('small')
	} else {
		println('none')
	}
	match count {
		1, 2 { println('few') }
		else { println('many') }
	}
	bytes := names[0].bytes()
	total := apply(fn (x int) int {
		return x * 2
	}, count)
	println(bytes.len)
	println(total)
	println(q.x)
	println(grid[0])
	println(scores['a'])
	println(make_point(1, 2).sum())
}
