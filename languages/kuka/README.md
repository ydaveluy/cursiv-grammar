# kuka

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
| The grammar | `src/kuka/core/KukaGrammar.hpp` |
| The AST | `src/kuka/core/ast.hpp` |
| Validation checks | `src/kuka/core/validation/` |
| Services of your own | `src/kuka/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/kuka/cli/main.cpp` |
| LSP features | `src/kuka/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
DEF example()
DECL INT i
DECL POS cpos
DECL AXIS jpos
FOR i=1 TO 6
$VEL_AXIS[i]=60
ENDFOR
jpos = {AXIS: A1 0,A2 -90,A3 90,A4 0,A5 0,A6 0}
PTP jpos
IF $IN[1] == TRUE THEN
cpos = {POS: X 300,Y -100,Z 1500,A 0,B 90,C 0}
ELSE
cpos = {POS: X 250,Y -200,Z 1300,A 0,B 90,C 0}
ENDIF
END
```

KRL, the KUKA Robot Language: keywords are case-insensitive, a newline ends
a statement, and a struct literal like `{AXIS: A1 0, ...}` is how a robot
position or joint set is written inline.

## The command line

```bash
./build/src/kuka_cli example/example1.kuka
./build/src/kuka_cli --dump example/example1.kuka   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`kuka_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
