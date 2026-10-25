# lucene

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
| The grammar | `src/lucene/core/LuceneGrammar.hpp` |
| The AST | `src/lucene/core/ast.hpp` |
| Validation checks | `src/lucene/core/validation/` |
| Services of your own | `src/lucene/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/lucene/cli/main.cpp` |
| LSP features | `src/lucene/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
title:"The Right Way" AND text:go
mod_date:[20020101 TO 20030101]
jakarta^4 apache
roam~0.8
(jakarta OR apache) AND website
```

A query is a list of clauses, joined by `AND`/`OR` or by nothing at all (an
implicit OR). A clause is a bare term, a `field:value` or `field=value` pair,
a `field<value`/`field<=value`/... range comparison, a `[lo TO hi]` or
`{lo TO hi}` range, or a parenthesised sub-query — each optionally prefixed
by `+` (required), `-`/`NOT`/`!` (excluded), and optionally suffixed by
`~similarity` (fuzzy) or `^boost`.

## The command line

```bash
./build/src/lucene_cli example/boolean-1.lucene
./build/src/lucene_cli --dump example/boolean-1.lucene   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`lucene_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
