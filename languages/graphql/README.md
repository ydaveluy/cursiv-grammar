# graphql

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
| The grammar | `src/graphql/core/GraphqlGrammar.hpp` |
| The AST | `src/graphql/core/ast.hpp` |
| Validation checks | `src/graphql/core/validation/` |
| Services of your own | `src/graphql/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/graphql/cli/main.cpp` |
| LSP features | `src/graphql/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

The `.g4` this port draws from ships no `examples/` directory, so `example/`
is empty; `tests/query.graphql` and `tests/schema.graphql` are this port's
own corpus instead.

```graphql
query GetUser($id: ID!) {
  user(id: $id) {
    id
    name: fullName
    ...UserFields
  }
}
```

covers the executable query language: operations, variables, aliases,
fragment spreads. `tests/schema.graphql` covers the type-system definition
language beside it -- `schema`, every type kind, a directive definition, and
every extension kind.

## The command line

```bash
./build/src/graphql_cli tests/query.graphql
./build/src/graphql_cli --dump tests/query.graphql   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`graphql_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
