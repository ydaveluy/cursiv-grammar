# sqlite

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
| The grammar | `src/sqlite/core/SqliteGrammar.hpp` |
| The AST | `src/sqlite/core/ast.hpp` |
| Validation checks | `src/sqlite/core/validation/` |
| Services of your own | `src/sqlite/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/sqlite/cli/main.cpp` |
| LSP features | `src/sqlite/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```sql
-- a common table expression feeding an aggregate query
WITH totals AS (
  SELECT customer_id, SUM(amount) AS spent
    FROM orders
   GROUP BY customer_id
)
SELECT c.name, t.spent
  FROM customers AS c
  JOIN totals AS t ON t.customer_id = c.id
 WHERE t.spent > 100
 ORDER BY t.spent DESC
 LIMIT 10;
```

This is SQLite's dialect: the twelve-level expression grammar (`IS`,
`BETWEEN`, `LIKE`, window functions, `->`/`->>` JSON arrows...), statements
including DDL, DML, `PRAGMA`, `ATTACH`, transactions, triggers and virtual
tables, quoted identifiers in four spellings, and SQLite's own
case-insensitive, mostly-unreserved keyword set. An arbitrary parenthesised
run inside a `CREATE VIRTUAL TABLE` module argument is not supported.

## The command line

```bash
./build/src/sqlite_cli example/cte.sql
./build/src/sqlite_cli --dump example/cte.sql   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`sqlite_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
