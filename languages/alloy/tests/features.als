// Exercises constructs fact.als/fs.als/sig.als do not: module/open,
// abstract sig, sigExt 'in', pred/fun with both paraDecls brackets, box
// join, arrow multiplicities, every quantifier, let, a set comprehension,
// every unary and binary operator, every (negated) compare operator, the
// if/else form, every const_, '@' and 'this', and every cmdDecl/scope shape.
module features/demo [M]

open util/ordering as ord
open util/graph, util/relation

abstract sig Object {}
abstract sig Shape in Object {}
sig Circle, Square extends Shape {}

sig A {
  r: A one -> some A,
  disj x, y: A,
  z: disj (A + A)
}

sig B { }

fact Quantifiers {
  all a: A | a = a
  no a: A | a != a
  lone a: A | a = a
  some a: A | a = a
  one a: A | a = a
  sum a: A | #a
}

fact LetAndCompr {
  let s = A | some s
  let s = A { some s }
  #{ a: A | a = a } >= 0
}

fact UnaryOps {
  ! (some A)
  not (some A)
  no A
  lone A
  some A
  one A
  set A
  #A = #A
  ~r
  *r
  ^r
}

fact BinaryOps {
  (some A) || (no B)
  (some A) or (no B)
  (some A) && (some B)
  (some A) and (some B)
  (some A) <=> (some B)
  (some A) iff (some B)
  A & A = A & A
  A + A = A + A
  A - A = A - A
  A ++ A = A ++ A
  A <: A = A <: A
  A :> A = A :> A
  A.r = A.r
}

fact ArrowAndBoxJoin {
  A -> A = A -> A
  A one -> A = A one -> A
  A -> some A = A -> some A
  r[x] = r[x]
  r[x, y] = r[x][y]
}

fact CompareOps {
  A in A
  A !in A
  A not in A
  A = A
  A != A
  A not = A
  A < A
  A !< A
  A > A
  A !> A
  A =< A
  A !=< A
  A >= A
  A !>= A
}

fact IfElse {
  (some A) => (some B) else (no B)
  (some A) implies (some B) else (no B)
}

fact Consts {
  none = none
  univ = univ
  iden = iden
  -1 = -1
  @r = @r
}

pred paren(disj x, y: A) {
  x = y => (no A) else this = this
}

pred bracket[x: A] {
  x in A
}

fun withReturn[x: A]: A { x.r }

run paren for 3
run bracket for exactly 3 A
check withReturn for 3 but 2 A, 1 B
myCmd: run { some A } for 5
