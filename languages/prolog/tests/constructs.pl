% Exercises every construct the reference grammar accepts.
:- dynamic counter/1.
:- multifile hook/2.
:- discontiguous foo/1.
:- public bar/0.

/* A block comment /* nested */ still closes correctly. */

fact(X) :- X > 0, X < 100, Y is X * 2 + 1, Y mod 3 =:= 0.

greeting('Hello, world').
greeting("double quoted").
greeting(`back quoted`).

quoted_continuation('line one \
line two').

escapes('tab\there quote\' backslash\\ hex\x41\ octal\101\ newline\n').

pair(A, B) :- A = B ; A \= B.

member_(X, [X|_]).
member_(X, [_|T]) :- member_(X, T).

empty_list([]).
singleton([a]).
curly({a,b,c}).
empty_braces({}).

negatives(-1, -3.14).
radixes(0b101, 0o17, 0x1F, 0'a).

cut_test(X) :- X > 0, !, X < 10.

not_test(X) :- \+ member_(X, [1,2,3]).

order_test(X, Y) :- X @< Y, X @=< Y, X @> Y, X @>= Y.
arith_cmp_test(X, Y) :- X =:= Y, X =\= Y.

univ_test(L) :- foo(1,2) =.. L.

module_test :- lists:append([1], [2], X).

shift_test(X) :- X is 5 << 2.
shift_test2(X) :- X is 20 >> 2.
bitops_test(X) :- X is 5 /\ 3.
bitops_test2(X) :- X is 5 \/ 2.
pow_test(X) :- X is 2 ** 8.
intdiv_test(X) :- X is 7 // 2.
complement_test(X) :- X is \ 5.

dcg_test --> [hello], [world].

graphic_functor(X, Y) :- #=(X, Y).

?- true.
