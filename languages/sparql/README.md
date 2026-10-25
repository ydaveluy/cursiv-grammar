# sparql

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
| The grammar | `src/sparql/core/SparqlGrammar.hpp` |
| The AST | `src/sparql/core/ast.hpp` |
| Validation checks | `src/sparql/core/validation/` |
| Services of your own | `src/sparql/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/sparql/cli/main.cpp` |
| LSP features | `src/sparql/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```sparql
PREFIX dc: <http://purl.org/dc/elements/1.1/>
SELECT ?node ?title
WHERE{
  ?node dc:title ?title .
}
LIMIT 3
```

A SPARQL 1.0 query: a prologue of `BASE`/`PREFIX` declarations, then one of
`SELECT`/`CONSTRUCT`/`DESCRIBE`/`ASK`, a `WHERE` graph pattern of triples,
`OPTIONAL`/`UNION`/`GRAPH`/`FILTER`, and a solution modifier (`ORDER BY`,
`LIMIT`, `OFFSET`). A `PrefixedTerm` such as `dc:title` is not a cross
reference to a definition site (SPARQL has none): the validator instead
walks the tree by hand and flags a prefix that no `PREFIX` declared.

## The command line

```bash
./build/src/sparql_cli example/example4.rq
./build/src/sparql_cli --dump example/example4.rq   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`sparql_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
