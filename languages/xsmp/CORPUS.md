# Corpus

Real ECSS SMP standard catalogues, copied verbatim from xsmp-modeler
(`packages/xsmp/src/builtins/`). Real input rather than synthetic, so the
tests hold the grammar to what it actually meets.

| file | bytes | note |
|---|---|---|
| `ecss_smp_l1.xsmpcat`   | 240 619 | ECSS SMP 2025 level 1 |
| `ecss_smp_2020.xsmpcat` | 192 091 | ECSS SMP 2020 |

Both are heavily doc-commented (`/** ... */`), so a large share of the input
is trivia: the skipper crosses far more of it than the declarations do.

`tests/grammar_test.cpp` pins both catalogues -- bytes, tree size, hidden
leaves -- and `ecss_smp_2020.xsmpcat`'s AST by value: one count per node
class, and what each cited name picks out, by source line.
`tests/linking_test.cpp` builds each catalogue alone and requires no
diagnostic. A catalogue edit turns the grammar test red.

## GRAMMAR.txt

The grammar as the generator hands it to the parser is written with the rest
of the generated tree, and `tests/grammar_source_test.cpp` holds the
language's grammar source to it. The target that writes it:

```
cmake --build build --target xsmp_generate    # rewrite
```

It is build output like the rest, so reading a pass off it means keeping a
copy across the change: a pass that reshapes a rule shows up on a few lines,
and a pass that quietly STOPS FIRING shows up at all.
