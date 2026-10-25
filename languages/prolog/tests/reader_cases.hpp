#pragma once
struct ReaderCase { const char* name; const char* source; };
inline constexpr ReaderCase readerCases[] = {
  {"basic", R"PL9(foo(a, X, X, _). empty([]). brace({}). cut(!).)PL9"},
  {"precedence-prefix", R"PL9(-a^b. -2^2. - 2^2. a^b^c. a^b**c. (-a)**b. a**(-b).)PL9"},
  {"precedence-branch", R"PL9(p :- a,b -> c ; d. p :- a *-> b ; c. (a,b|c).)PL9"},
  {"precedence-compare", R"PL9(A =@= B. A \=@= B. A >:< B. A :< B. M:a := b. a as b.)PL9"},
  {"operators-arithmetic", R"PL9(X is 10 div 3. X is 7 rdiv 2. X is 2 xor 3. X is 1 /\ 2 \/ 3.)PL9"},
  {"directives", R"PL9(:- dynamic a/1. :- volatile a/1. :- thread_local a/1. :- initialization(main). :- thread_initialization(main). :- module_transparent a/1. :- meta_predicate p(0). :- table a/1. ?- true.)PL9"},
  {"functors", R"PL9(f(). '+'(a,b). -(1). f((+), (-), (;), (!)).)PL9"},
  {"lists", R"PL9(f([a,b|T], [a|(b,c)], [[],[1,2]], [+, -]).)PL9"},
  {"curly", R"PL9({a,b,c}. {a;b}. {(a,b)}. f({a,b}).)PL9"},
  {"quoted-atoms", R"PL9(f('éλ😀', 'can''t', 'quote\'slash\\', '\u03bb\U0001F600', '\x41\ \101\').)PL9"},
  {"quoted-relaxedhex", R"PL9(f('\x41!', '\101!').)PL9"},
  {"quoted-controls", R"PL9(f('\a\b\e\f\n\r\s\t\v').)PL9"},
  {"quoted-continuation", R"PL9(f('left\
   right', 'left\c 
  right').)PL9"},
  {"quoted-newline", R"PL9(f('line
next').)PL9"},
  {"strings", R"PL9(f("a""b", "é\u03bb\U0001F600", "zero\x0\end", `ab😀`).)PL9"},
  {"unicode-identifiers", R"PL9(école(École, école, Λ, λ, 变量, _变量, Aé, é).)PL9"},
  {"unicode-symbols", R"PL9(f(⊕, ≤, →).)PL9"},
  {"floats", R"PL9(f(1e3, 1.2e3, 1.2E-3, 2e+2, -2.5e2).)PL9"},
  {"float-specials", R"PL9(f(1.0Inf, -1.0Inf, 1.5NaN).)PL9"},
  {"float-unicode", R"PL9(f(٢.٥e٢, -٢.٥e٢, ١٢٣).)PL9"},
  {"integers", R"PL9(f(0b101, 0o77, 0xABC, 16'ff, 36'z, 2'101).)PL9"},
  {"integer-groups", R"PL9(f(1_000_000, 1 000 000, 0xAB_CD, 12_
 345).)PL9"},
  {"big-integers", R"PL9(f(1234567890123456789012345678901234567890, -123456789012345678901234567890).)PL9"},
  {"character-codes", R"PL9(f(0'a, 0'λ, 0'\n, 0'\u03bb, 0'\x41\, 0''').)PL9"},
  {"rationals", R"PL9(f(1r2, -10r4, 2r1).)PL9"},
  {"dcg", R"PL9(sentence(X) --> [hello], {goal(X), !}, rest(X).)PL9"},
  {"dicts", R"PL9(f(tag{b:2,a:1}, _{name:"value",1:x}, Tag{a: V}).)PL9"},
  {"dict-access", R"PL9(D.key. D.get(key). D.a.b. D := _{a: 1}.)PL9"},
  {"quasi", R"PL9(f({|html(X)||  raw % comment
"quotes" {text} |}).)PL9"},
  {"flags-quotes", R"PL9(:- set_prolog_flag(double_quotes, codes). f("ab").)PL9"},
  {"precedence-arithmetic-valid", R"PL9(p :- X = 1-2-3, Y is 1+2*3, Z is 2^(3**4).)PL9"},
  {"unicode-whitespace-valid", R"PL9(a. b. c. d.)PL9"},
  {"comments-valid", R"PL9(a /* nested /* c */ d */ . b.%line
c. /*end*/
)PL9"},
  {"character-extra", R"PL9(f(0'\c,0'\e,0'\s,0'',-0'λ).)PL9"},
  {"nil-vs-atom", R"PL9(f([], '[]', '', 'a', a).)PL9"},
  {"float-underflow", R"PL9(f(1e-999).)PL9"},
  {"float-nan-payload", R"PL9(f(1.25NaN, 1.5NaN, -1.75NaN).)PL9"},
  {"empty-comment", R"PL9(f([/*c*/], { %line
 }, [ /*c*/ ](), '[]'()).)PL9"},
};
