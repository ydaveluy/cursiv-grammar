# focal

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
| The grammar | `src/focal/core/FocalGrammar.hpp` |
| The AST | `src/focal/core/ast.hpp` |
| Validation checks | `src/focal/core/validation/` |
| Services of your own | `src/focal/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/focal/cli/main.cpp` |
| LSP features | `src/focal/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

FOCAL: every line opens with a group and line number, then a command.

```
01.10 ASK "HOW MUCH MONEY DO YOU WANT TO BORROW ?",PRINCIPAL
01.20 ASK "FOR HOW MANY YEARS ?",TERM
01.30 FOR RATE=4.0,.5,10;DO 2.0
01.40 QUIT
02.10 SET INTEREST=PRINCIPAL*(RATE/100)*TERM
02.20 TYPE "RATE",RATE,"   ","INTEREST",INTEREST,!
```

`DO 2.0` jumps to line `02.10` and runs until the group ends; `FOR` reruns the
body once per step of its loop variable; `TYPE`'s `!` prints a newline.

## The command line

```bash
./build/src/focal_cli example/example1.focal
./build/src/focal_cli --dump example/example1.focal   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`focal_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
