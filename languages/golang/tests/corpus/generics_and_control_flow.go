package main

import (
	"fmt"
	f "fmt"
)

type Number interface {
	~int | ~int64 | ~float64
}

func Sum[T Number](xs ...T) T {
	var total T
	for _, x := range xs {
		total += x
	}
	return total
}

type Animal interface {
	Sound() string
}

type Named struct {
	Name string `json:"name"`
}

type Dog struct {
	Named
	*Owner
	legs int
}

type Owner struct {
	Name string
}

func (d Dog) Sound() string { return "Woof" }

func describe(a Animal) string {
	switch v := a.(type) {
	case Dog:
		return v.Name
	case nil:
		return "nothing"
	default:
		return "unknown"
	}
}

func variadicSum(nums ...int) int {
	total := 0
	for _, n := range nums {
		total += n
	}
	return total
}

func classify(x int) string {
	switch {
	case x < 0:
		return "negative"
	case x == 0:
		return "zero"
	default:
		return "positive"
	}
}

func withLabels() {
	i := 0
Loop:
	for i < 10 {
		if i == 5 {
			goto Done
		}
		i++
		continue Loop
	}
Done:
	fmt.Println(i)
}

func withSelect(a, b chan int, done chan struct{}) {
	select {
	case v := <-a:
		fmt.Println(v)
	case b <- 1:
	case <-done:
		return
	default:
	}
}

const (
	_ = iota
	KB = 1 << (10 * (iota))
	MB
	GB
)

type Matrix [3][3]float64
type Handler func(w int, r string) error
type StringSet map[string]struct{}

func nested() {
	m := map[string][]int{
		"a": {1, 2, 3},
		"b": {4, 5, 6},
	}
	grid := [2][2]int{{1, 2}, {3, 4}}
	_ = m
	_ = grid
	var anon struct {
		X, Y int
	}
	anon.X = 1
	p := &anon
	_ = p
}

func typeAssertAndConvert(x interface{}) {
	n, ok := x.(int)
	_ = ok
	y := float64(n)
	z := []byte("hello")
	_ = y
	_ = z
	var r rune = 'é'
	var raw = `a raw
string`
	_ = r
	_ = raw
}

func multiReturn() (a int, b string, err error) {
	return 1, "two", nil
}

func closures() func() int {
	count := 0
	return func() int {
		count++
		return count
	}
}

func init() {
	_ = f.Sprintf
}

func main() {
	fmt.Println(Sum(1, 2, 3))
	describe(Dog{})
	variadicSum(1, 2, 3)
	classify(0)
	withLabels()
	ch1 := make(chan int, 1)
	ch2 := make(chan int, 1)
	done := make(chan struct{})
	withSelect(ch1, ch2, done)
	nested()
	typeAssertAndConvert(5)
	multiReturn()
	closures()
	var s StringSet = map[string]struct{}{"x": {}}
	_ = s
	var h Handler
	_ = h
	var mtx Matrix
	_ = mtx
}
