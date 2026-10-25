# pdn

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
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
