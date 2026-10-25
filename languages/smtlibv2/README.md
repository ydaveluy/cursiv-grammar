# smtlibv2

A language library built with [cursiv](https://github.com/ydaveluy/cursiv).

## Build

```bash
cmake -B build
cmake --build build -j
ctest --test-dir build --output-on-failure
```

The build generates the parser automatically. Rebuild after changing the grammar.

## What to edit

| What you want to change | File |
|---|---|
| The grammar | `src/smtlibv2/core/Smtlibv2Grammar.hpp` |
| The AST | `src/smtlibv2/core/ast.hpp` |
| Validation checks | `src/smtlibv2/core/validation/` |
| Services of your own | `src/smtlibv2/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/smtlibv2/cli/main.cpp` |
| LSP features | `src/smtlibv2/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```smt2
(set-logic QF_LIA)
(declare-const x Int)
(declare-const y Int)
(assert (= (+ x y) 10))
(assert (> x 0))
(check-sat)
(get-value (x y))
```

A document is one of four things: a script of commands (above), a `logic`
declaration, a `theory` declaration, or a solver response such as `sat` or
`(error "...")` — the four shapes `ast::Doc` covers. `declare-const` and
`assert` build `ast::Command` nodes; `check-sat` and `get-value` do too, kept
in `ast::Script::commands` in source order.

## The command line

```bash
./build/src/smtlibv2_cli example/test1.smt2
./build/src/smtlibv2_cli --dump example/test1.smt2   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`smtlibv2_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
