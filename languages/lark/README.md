# lark

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
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
