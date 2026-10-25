# pl0

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
| The grammar | `src/pl0/core/Pl0Grammar.hpp` |
| The AST | `src/pl0/core/ast.hpp` |
| Validation checks | `src/pl0/core/validation/` |
| Services of your own | `src/pl0/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/pl0/cli/main.cpp` |
| LSP features | `src/pl0/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

[PL/0](https://en.wikipedia.org/wiki/PL/0) is Wirth's teaching language: a
`CONST`/`VAR`/`PROCEDURE` header, one statement, and nothing else.

```
VAR x, squ;

PROCEDURE square;
BEGIN
   squ:= x * x
END;

BEGIN
   x := 1;
   WHILE x <= 10 DO
   BEGIN
      CALL square;
      ! squ;
      x := x + 1
   END
END.
```

`CALL` invokes a procedure, `?x` reads a value into `x`, `!e` prints an
expression, `WRITE x` prints a variable. Keywords match in any case; names do
not, and hold letters only. `example/` contains PL/0 programs covering nested
parentheses, a leading sign, `>=` and an empty statement.

## The command line

```bash
./build/src/pl0_cli example/example1.pl0
./build/src/pl0_cli --dump example/example1.pl0   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`pl0_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
