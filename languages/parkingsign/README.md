# parkingsign

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
| The grammar | `src/parkingsign/core/ParkingsignGrammar.hpp` |
| The AST | `src/parkingsign/core/ast.hpp` |
| Validation checks | `src/parkingsign/core/validation/` |
| Services of your own | `src/parkingsign/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/parkingsign/cli/main.cpp` |
| LSP features | `src/parkingsign/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

A stream of Los Angeles parking-sign texts, one sign after another, the shape
a text-recognition pass over sign photos would hand you:

```
NO PARKING 9AM TO 12 NOON MONDAY STREET CLEANING
NO STOPPING 7AM TO 9AM 4PM TO 7PM EXCEPT SATURDAY & SUNDAY
2 HOUR PARKING 8AM TO 6PM EXCEPT SUNDAY
TOW-AWAY TEMPORARY NO PARKING 6:30AM TO 4PM EXCEPT SATURDAY & SUNDAY
DISTRICT NO. 78 PERMITS EXEMPT
```

Each line is one sign, its kind read off the words alone (`NO PARKING`, `NO
STOPPING`, `N HOUR PARKING`, `PERMITS`, …); the document is the sequence of
signs a single pole carries.

## The command line

```bash
./build/src/parkingsign_cli example/example1.parkingsign
./build/src/parkingsign_cli --dump example/example1.parkingsign   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`parkingsign_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
