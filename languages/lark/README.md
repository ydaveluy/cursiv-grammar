# lark

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
| The grammar | `src/lark/core/LarkGrammar.hpp` |
| The AST | `src/lark/core/ast.hpp` |
| Validation checks | `src/lark/core/validation/` |
| Services of your own | `src/lark/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/lark/cli/main.cpp` |
| LSP features | `src/lark/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

`lark` parses [Lark](https://github.com/lark-parser/lark)'s own grammar
format: the `.lark` files that describe a rule- and token-based grammar,
templates included.

```
start: r
r.-1: "a"
```

A rule declares its priority with `.N` (negative allowed), a token with the
same syntax; `%import`, `%ignore`, `%declare` and `%override` are the four
statement forms, and `name{a, b}: ...` a template. See `example/` for the
project's own self-describing grammar (`lark.lark`) and a real-world one
(`python3.lark`).

## The command line

```bash
./build/src/lark_cli example/json.lark
./build/src/lark_cli --dump example/json.lark   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`lark_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
