# bcpl

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
| The grammar | `src/bcpl/core/BcplGrammar.hpp` |
| The AST | `src/bcpl/core/ast.hpp` |
| Validation checks | `src/bcpl/core/validation/` |
| Services of your own | `src/bcpl/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/bcpl/cli/main.cpp` |
| LSP features | `src/bcpl/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
GET "libhdr"

LET start() = VALOF
{ FOR i = 0 TO 16 DO writef("%n! = %n*n", i, factorial(i))
  RESULTIS 0
}

AND factorial(n) = n=0 -> 1, n*factorial(n-1)
```

`GET` names a section to bring in — a directive, not a resolved reference: no
symbol table follows it. `LET ... AND ...` binds `start` and `factorial`
simultaneously, so each may call the other. `n=0 -> 1, n*factorial(n-1)` is
BCPL's conditional expression: `cond -> then, else`.

## The command line

```bash
./build/src/bcpl_cli example/fact.bcpl
./build/src/bcpl_cli --dump example/fact.bcpl   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`bcpl_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
