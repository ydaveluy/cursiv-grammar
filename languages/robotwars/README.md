# robotwars

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
| The grammar | `src/robotwars/core/RobotwarsGrammar.hpp` |
| The AST | `src/robotwars/core/ast.hpp` |
| Validation checks | `src/robotwars/core/validation/` |
| Services of your own | `src/robotwars/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/robotwars/cli/main.cpp` |
| LSP features | `src/robotwars/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

[RobotWar](http://corewar.co.uk/robotwar.htm) programs a robot with a small,
line-oriented instruction set: move it, aim it, fire, and branch on what its
radar and damage sensor read.

```
;     ROBOT  'SCANNER'

; SEND RADAR PULSE AND FIRE IF AN
; ENEMY IS SPOTTED

SCAN
   AIM + 7 TO AIM

CHECK
   AIM TO RADAR
   IF RADAR > 0 GOTO SCAN
   0 - RADAR TO SHOT
   GOTO CHECK
```

A bare word starting a line is a label (`SCAN`, `CHECK`); `GOTO`/`GOSUB` name
one — a cross reference the linker resolves, which is what makes rename,
go-to-definition and "unresolved name" diagnostics work at all. `TO` writes
an expression into one or more registers; `IF` guards a statement on a
comparison.

## The command line

```bash
./build/src/robotwars_cli example/scanner.robotwars
./build/src/robotwars_cli --dump example/scanner.robotwars   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`robotwars_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
