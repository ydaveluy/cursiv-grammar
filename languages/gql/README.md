# gql

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
| The grammar | `src/gql/core/GqlGrammar.hpp` |
| The AST | `src/gql/core/ast.hpp` |
| Validation checks | `src/gql/core/validation/` |
| Services of your own | `src/gql/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/gql/cli/main.cpp` |
| LSP features | `src/gql/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

GQL (ISO/IEC 39075) is a SQL-shaped query language over property graphs:

```
MATCH (n:Person)-[f:FRIEND_OF]->(m:Person)
RETURN n.name, f.since, m.name
```

`MATCH` binds a graph pattern -- nodes in parens, edges in brackets, labels
after `:` -- and `RETURN` projects an expression per row, the way a SQL
`SELECT` does. The port also covers `INSERT`/`SET`/`REMOVE`/`DELETE`,
`LET`/`FOR`/`FILTER`, composite queries (`UNION`/`EXCEPT`/`INTERSECT`), a
`SELECT` form, and the catalog DDL (`CREATE`/`DROP SCHEMA`/`GRAPH`/`GRAPH
TYPE`) and session/transaction commands that frame a program.

## The command line

```bash
./build/src/gql_cli example/01_simple_match.gql
./build/src/gql_cli --dump example/01_simple_match.gql   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`gql_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
