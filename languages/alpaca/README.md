# alpaca

An [ALPACA](https://catseye.tc/article/Languages.md#alpaca) cellular-automaton
description language library built with
[cursiv](https://github.com/ydaveluy/cursiv).

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
| The grammar | `src/alpaca/core/AlpacaGrammar.hpp` |
| The AST | `src/alpaca/core/ast.hpp` |
| Validation checks | `src/alpaca/core/validation/` |
| Services of your own | `src/alpaca/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/alpaca/cli/main.cpp` |
| LSP features | `src/alpaca/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
state Dead  " " to Alive when 3 Alive and 5 Dead;
state Alive "*" to Dead when 4 Alive or 7 Dead.
```

An ALPACA document declares `state`s, `class`es and `neighbourhood`s,
separated by `;` and closed by `.` or `begin`. A state carries a display
glyph and transition rules (`to <state> when <condition>`); a condition
counts neighbours matching a state or class, tests equality between two
state references, or combines those with `and`/`or`/`xor`/`not`.

## The command line

```bash
./build/src/alpaca_cli example/conway.alp
./build/src/alpaca_cli --dump example/conway.alp   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`alpaca_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
