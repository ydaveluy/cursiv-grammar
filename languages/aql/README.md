# aql

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
| The grammar | `src/aql/core/AqlGrammar.hpp` |
| The AST | `src/aql/core/ast.hpp` |
| Validation checks | `src/aql/core/validation/` |
| Services of your own | `src/aql/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/aql/cli/main.cpp` |
| LSP features | `src/aql/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

AQL (ArangoDB Query Language): a read chains `FOR`/`FILTER`/`SORT`/`LIMIT`/
`COLLECT`/`WINDOW` clauses into a `RETURN`, or walks a graph; a write is
`INSERT`/`UPDATE`/`REPLACE`/`REMOVE`/`UPSERT`.

```
FOR u IN users
  COLLECT country = u.country, city = u.city INTO groups = {
    "name" : u.name,
    "isActive" : u.status == "active"
  }
  RETURN {
    "country" : country,
    "city" : city,
    "usersInCity" : groups
  }
```

No name is resolved against a definition here: a `FOR`/`LET` binding is
read back as text, not linked, since nothing in the document declares a
collection's shape for a reference to check against.

## The command line

```bash
./build/src/aql_cli example/collect_1.aql
./build/src/aql_cli --dump example/collect_1.aql   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`aql_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
