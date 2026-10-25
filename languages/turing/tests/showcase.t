% Exercises every construct the grammar accepts beyond the two short corpus
% samples: declarations, types, subprograms, control flow, I/O and the full
% expression grammar.

const Pi : real := 3.14159e0
const Salutation := "hello"
const Trio : array 1..3 of int := init(1, 2, 3)
const Nested : array 1..2 of array 1..2 of int := init(init(1, 2), init(3, 4))

type Point : record x, y : int end record
type Digit : 0..9
type Row : array 1..3 of int
type Age : nat

var count, total : int
var ratio := 1.5
var flag : boolean := true
var label_ : string(5)
var p : Point
var grid : array 1..2, 1..2 of int
var text := "hello world"

procedure Greet (name : string)
    put "Hello, ", name, "!"
end Greet

function Square (n : int) : int
    result n * n
end Square

procedure Fill (var arr : array 1..* of int, n : int)
    var i : int
    for i : 1 .. n
        arr (i) := i * i
    end for
end Fill

procedure Show (s : string(*))
    put s
end Show

p.x := 1
p.y := 2
put p.x, p.y

grid (1, 1) := 9
put grid (1, 1)

count := 0
loop
    exit when count >= 3
    count := count + 1
end loop

if count = 3 then
    put "reached three"
elsif count > 3 then
    put "overshot"
else
    put "under"
end if

case count of
    label 0: put "zero"
    label 1, 2: put "one or two"
    label: put "other"
end case

for count : 1 .. 3 by 1
    put count : 4
end for

for decreasing count : 3 .. 1
    put count
end for

total := Pi * 2 ** 3 - 4 / 2 div 1 mod 3
assert total not= 0
put total : 10 : 2 : 1

put text (1 .. 5)
put text (* - 5 .. *)
put text (1)

flag := (1 < 2) and (3 >= 2) or not (1 = 2)
put flag

Greet ("world")
put Square (count)

put : 1, "on stream one"
get : 1, count
put skip
put "trailing, no newline" ..

open : 1, "data.t", get, put
close : 1
