; numbers, keywords and characters
[42 -7 3.14 -1.5e10 0x1F 2r101 100N 5]
[:foo ::bar nil true false]
[\a \newline \u0041]

; sets and maps
#{1 2 3}
{:a 1 :b 2}

; quoting family
'foo
`foo
~foo
~@foo

; deref and tag metadata
@foo
^String foo

; var-quote, tagged literal, discard, dispatch, regex
#'foo
#+foo bar
#_foo bar
#inst "2020-01-01"
#"foo.*bar"

; gensym and anonymous function params
(let [x# 1] x#)
#(+ % %1 %2 %&)
