# turing

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
| The grammar | `src/turing/core/TuringGrammar.hpp` |
| The AST | `src/turing/core/ast.hpp` |
| Validation checks | `src/turing/core/validation/` |
| Services of your own | `src/turing/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/turing/cli/main.cpp` |
| LSP features | `src/turing/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

[Turing](https://en.wikipedia.org/wiki/Turing_(programming_language)) is
Ontario's teaching language: Pascal-like declarations and control flow, plus
`put`/`get` for formatted I/O and `1-indexed` substrings.

```turing
var name : string
put "What is your name?"
get name
put "Hello, ", name, "!"
```

`example/` also carries `showcase.t`, which exercises constants, types
(standard, subrange, array, record, named), procedures and functions,
`if`/`case`/`for`/`loop`, and the full expression grammar including
`init(...)` and substrings.

## The command line

```bash
./build/src/turing_cli example/example1.t
./build/src/turing_cli --dump example/example1.t   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`turing_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
