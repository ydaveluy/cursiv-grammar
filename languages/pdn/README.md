# pdn

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
| The grammar | `src/pdn/core/PdnGrammar.hpp` |
| The AST | `src/pdn/core/ast.hpp` |
| Validation checks | `src/pdn/core/validation/` |
| Services of your own | `src/pdn/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/pdn/cli/main.cpp` |
| LSP features | `src/pdn/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

[PDN](https://en.wikipedia.org/wiki/Portable_Draughts_Notation) records a
game of draughts (checkers): a set of `[Tag "value"]` pairs, then the moves.

```
[Event "The Royal Tour"]
[White "b"]
[Black "a"]
[Result "1-0"]

1. 19-15 10x19 2. 5-1 3x10 {a capture} 1-0
```

A move is a square-to-square slide (`19-15`) or a jump (`10x19`); the game
ends in a result (`1-0`, `0-1`, `1/2-1/2`) or `*` for one still open.

## The command line

```bash
./build/src/pdn_cli example/example1.pdn
./build/src/pdn_cli --dump example/example1.pdn   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`pdn_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
