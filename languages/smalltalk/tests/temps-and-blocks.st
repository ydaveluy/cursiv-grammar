| sum items |
sum := 0.
items := #(1 2 3 4 5).
items do: [:each | sum := sum + each].
^sum
