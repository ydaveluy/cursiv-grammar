# mumps

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
| The grammar | `src/mumps/core/MumpsGrammar.hpp` |
| The AST | `src/mumps/core/ast.hpp` |
| Validation checks | `src/mumps/core/validation/` |
| Services of your own | `src/mumps/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/mumps/cli/main.cpp` |
| LSP features | `src/mumps/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
FIBONACCI ; compute the Fibonacci series
 SET (A,B)=1
 FOR I=1:1 SET S=A+B WRITE !,S SET A=B SET B=S QUIT:S>100
 WRITE !,"Result= ",S
```

A line either opens with a label at column one (`FIBONACCI`) or is indented
by at least one space; `!` in a WRITE argument list is a newline, and a
command's `:condition` is its postconditional -- `QUIT:S>100` quits the FOR
once `S` passes 100.

## The command line

```bash
./build/src/mumps_cli example/fibonacci.m
./build/src/mumps_cli --dump example/fibonacci.m   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`mumps_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
