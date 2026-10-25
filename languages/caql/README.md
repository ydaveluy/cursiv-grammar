# caql

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
| The grammar | `src/caql/core/CaqlGrammar.hpp` |
| The AST | `src/caql/core/ast.hpp` |
| Validation checks | `src/caql/core/validation/` |
| Services of your own | `src/caql/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/caql/cli/main.cpp` |
| LSP features | `src/caql/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

[CaQL](https://docs.circonus.com/caql/reference) is Circonus's metrics
query language: a vector expression piped through aggregation functions.

```
(find("dur2") | stats:sum()) * (find("dur2") | stats:sum()) + 2
```

`find(...)` selects a metric stream by name; `| stats:sum()` pipes it
through an aggregation. Arithmetic, comparison and set operators
(`+ - * / % ^`, `== != > < >= <=`, `and or unless`) combine expressions,
each optionally carrying an `on(...)`/`ignoring(...)` vector-matching
clause, as in PromQL.

## The command line

```bash
./build/src/caql_cli example/s1.caql
./build/src/caql_cli --dump example/s1.caql   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`caql_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
