package main

import "fmt"

type Pair[K comparable, V any] struct {
	Key K
	Val V
}

type Bar struct{ X int }

type Container[T any] struct {
	Item Bar
}

type Emb struct {
	Container[int]
}

func makeNamed() {
	type IntChan chan int
	ch := make(IntChan)
	_ = ch
	type IntSlice []int
	s := make(IntSlice, 0, 10)
	_ = s
}

func methodExprAndValue() {
	var d Dog
	f := d.Sound
	g := Dog.Sound
	_ = f
	_ = g
}

type Dog struct{}

func (d Dog) Sound() string { return "woof" }

func chanOfChan(c chan<- chan int) {
	var x <-chan chan int
	_ = x
	_ = c
}

func rangeForms(xs []int) {
	for range xs {
	}
	for i := range xs {
		_ = i
	}
	for i, v := range xs {
		_ = i
		_ = v
	}
}

func strEscapes() {
	s1 := "tab\there\nnewline\\backslash\"quote\x41é\U0001F600"
	s2 := `raw ` + "concat"
	_ = s1
	_ = s2
}

func main() {
	p := Pair[int, string]{Key: 1, Val: "a"}
	_ = p
	e := Emb{}
	_ = e
	makeNamed()
	methodExprAndValue()
	rangeForms([]int{1, 2, 3})
	strEscapes()
	fmt.Println("done")
}
