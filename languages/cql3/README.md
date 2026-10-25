# cql3

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
| The grammar | `src/cql3/core/Cql3Grammar.hpp` |
| The AST | `src/cql3/core/ast.hpp` |
| Validation checks | `src/cql3/core/validation/` |
| Services of your own | `src/cql3/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/cql3/cli/main.cpp` |
| LSP features | `src/cql3/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```sql
CREATE TABLE cycling.race_winners (
   race_name text,
   race_position int,
   cyclist_name FROZEN<fullname>,
   PRIMARY KEY (race_name, race_position));

SELECT id, lastname, teams
FROM cycling.cyclist_career_teams
WHERE id = 5b6962dd-3f90-4c93-8f61-eabfa4a803e2;

UPDATE cycling.cyclist_name
SET comments = 'Rides hard, gets along with others, a real winner'
WHERE id = fb372533-eb95-4bb4-8685-6ef61e994caa IF EXISTS;
```

This is Apache Cassandra's query language: DDL (`CREATE`/`ALTER`/`DROP` for
tables, keyspaces, types, indexes, materialized views, functions, aggregates,
roles, users, triggers), DML (`SELECT`, `INSERT`, `UPDATE`, `DELETE`,
`BATCH`), and the permission statements (`GRANT`, `REVOKE`, `LIST`). The
grammar holds no scope: names are plain text, not cross references — a query
routinely names tables and columns declared nowhere in the document.

## The command line

```bash
./build/src/cql3_cli example/select.cql3
./build/src/cql3_cli --dump example/select.cql3   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`cql3_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
