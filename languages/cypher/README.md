# cypher

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
| The grammar | `src/cypher/core/CypherGrammar.hpp` |
| The AST | `src/cypher/core/ast.hpp` |
| Validation checks | `src/cypher/core/validation/` |
| Services of your own | `src/cypher/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/cypher/cli/main.cpp` |
| LSP features | `src/cypher/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

[Cypher](https://opencypher.org/) is Neo4j's graph query language: patterns
of nodes and relationships stand in for the SQL `FROM`/`JOIN` a relational
query would need.

```
MATCH (node1:Label1)

WHERE node1.propertyA = $value

RETURN node2.propertyA, node2.propertyB
```

`(node1:Label1)` is a node pattern: `node1` binds the match, `:Label1`
constrains it. `$value` is a query parameter, bound outside the document.
Variable and property names are plain text here, not cross references: a
binding's declaration and its uses can sit in different query parts chained
by `WITH`, past what a single-document scope resolves reliably.

## The command line

```bash
./build/src/cypher_cli example/match1.cypher
./build/src/cypher_cli --dump example/match1.cypher   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`cypher_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
