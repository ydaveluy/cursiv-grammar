# kquery

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
| The grammar | `src/kquery/core/KqueryGrammar.hpp` |
| The AST | `src/kquery/core/ast.hpp` |
| Validation checks | `src/kquery/core/validation/` |
| Services of your own | `src/kquery/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/kquery/cli/main.cpp` |
| LSP features | `src/kquery/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

KLEE's KQuery: an S-expression format for constraint queries over bitvector
arrays -- array declarations, then query commands.

```
array sample_pse[4] : w32 -> w8 = symbolic

(query [(Ule N0:(ReadLSB w32 0 sample_pse) 4294967295)
        (Ule 6434 N0)]
       false)
```

`array` declares a symbolic or initialized bitvector array; a query command
is a list of constraints, a query expression, and optional eval lists. `N0:`
labels a subexpression so a later `N0` can refer back to it -- the only
naming in the language, and it needs no linker: a label reads back as the
same expression class as a bare reference, both plain text on the AST.

## The command line

```bash
./build/src/kquery_cli example/test1.kquery
./build/src/kquery_cli --dump example/test1.kquery   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`kquery_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
