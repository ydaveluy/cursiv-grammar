package main

import "fmt"

func sum(a int,
	b int) int {
	return a +
		b
}

func main() {
	x := 1 +
		2
	y := sum(
		1,
		2,
	)
	if x > 0 &&
		y > 0 {
		fmt.Println("ok")
	}
	s := []int{
		1,
		2,
		3,
	}
	m := map[string]int{
		"a": 1,
		"b": 2,
	}
	_ = s
	_ = m
	switch {
	case x > 0:
		fmt.Println("pos")
	}
	var z int
	z = x +
		y
	_ = z
	func() {
		fmt.Println("literal")
	}()
}
