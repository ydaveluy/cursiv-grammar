# Expression precedence regression checks

`grammar_test.cpp` checks expression trees as well as clean parsing. Its
duration, `ago`, `matches pattern`, string-prefix, arithmetic and concatenation
cases run unchanged with both Cursiv backends. Parent links are checked across
new wrappers and parentheses.

The LL grammar consumes the value of a match before nesting its suffix. Calling
`ExprStringPrefix` at the head of `ExprDurationMatches` would re-enter the same
rule through `ExprAgo` without consuming input. The pattern argument still reads
the wider string-expression rule.

There is one intentional compatibility limit: the port accepts permissive,
type-inconsistent chains containing `ago matches pattern`. Their association
now follows the same wider-pattern rule in LL and LR. For example, writing
`M(value, pattern)` for a match and `A(value)` for `ago`:

```text
a matches pattern b ago matches pattern y
old LR: M(A(M(a, b)), y)
LL/LR: M(a, M(A(b), y))
```

The [upstream ANTLR grammar, revision 4ed9403 (2024-09-19)](https://github.com/antlr/grammars-v4/blob/4ed9403a7ed0866c92e640c79139223ed0bc2051/arden/ArdenParser.g4#L395-L432)
permits only one unparenthesized match at the comparison level. The
[HL7/JAHIS Arden specification translation (2007)](https://www.jahis.jp/files/user/images/st07-101.pdf),
sections 9.8.4 (page 46) and 9.10.3 (page 51), specifies string operands and a
Boolean result for a match, and a duration operand and time result for `ago`.
These mixed chains cannot satisfy those signatures. The parser continues to
accept their tokens; this fix does not add a type checker.

The 2026-10-25 audit compared a frozen LR baseline against both corrected
backends on 1,408 generated expression probes using Cursiv `0562e6a` and GCC
16.2.1. The grammar and tests were also checked with Clang 22.1.2 and strict
warnings. All 1,334 clean parses and 74 rejections were preserved. LL and
LR agreed on every accepted tree. There were 196 changed trees relative to the
old LR baseline, all in the `ago matches pattern` family above. This is a bounded
comparison, not a proof of equivalence for every expression.
