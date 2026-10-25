# tl

A language library built with [cursiv](https://github.com/ydaveluy/cursiv).

## Build

Requires CMake 3.30+, Git and a C++20 compiler with `std::format` and `musttail`
support. Cursiv's CI uses GCC 16 and Clang 22.

```bash
cmake -B build
cmake --build build -j
ctest --test-dir build --output-on-failure
```

The build generates the parser automatically. Rebuild after changing the grammar.

## What to edit

| What you want to change | File |
|---|---|
| The grammar | `src/tl/core/TlGrammar.hpp` |
| The AST | `src/tl/core/ast.hpp` |
| Validation checks | `src/tl/core/validation/` |
| Services of your own | `src/tl/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/tl/cli/main.cpp` |
| LSP features | `src/tl/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

[TL](https://en.wikipedia.org/wiki/Temporal_logic#Prior's_tense_logic_(TL))
is Prior's minimal tense logic: atomic propositions, negation (`⌐`),
disjunction (`∨`), the falsity constant (`⊥`), and two temporal operators --
`G` ("always going to be") and `H` ("has always been"). A document is a
single formula, and the empty formula is itself valid, at the top level and
wherever a sub-formula is expected.

```
Gp ∨ q
```

reads "always p, or q" -- `G` is looser than `∨`, so it reaches across the
whole disjunction. Parentheses force a tighter reading: `G(p ∨ q)` and
`(Gp) ∨ q` differ.

## The command line

```bash
./build/src/tl_cli tests/precedence.tl
./build/src/tl_cli --dump tests/precedence.tl   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`tl_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
