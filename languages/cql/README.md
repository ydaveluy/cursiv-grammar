# cql

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
| The grammar | `src/cql/core/CqlGrammar.hpp` |
| The AST | `src/cql/core/ast.hpp` |
| Validation checks | `src/cql/core/validation/` |
| Services of your own | `src/cql/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/cql/cli/main.cpp` |
| LSP features | `src/cql/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

[CQL](https://www.loc.gov/standards/sru/cql/spec.html) (Contextual Query
Language, Z39.50's search syntax): a boolean tree of index/relation/term
clauses, `sortBy`, and `>` context-set prefix assignments.

```
dc.title any fish or (dc.creator any sanderson and dc.identifier = "id:1234567")
```

`dc.title any fish` is a search clause: an index, a relation (here the named
comparitor `any`), a term. Clauses combine with `and`/`or`/`not`/`prox`,
parenthesised for grouping; a relation or a boolean may carry `/modifier`s.

## The command line

```bash
./build/src/cql_cli example/example1.cql
./build/src/cql_cli --dump example/example1.cql   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`cql_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
