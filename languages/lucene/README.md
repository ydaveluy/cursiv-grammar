# lucene

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
| The grammar | `src/lucene/core/LuceneGrammar.hpp` |
| The AST | `src/lucene/core/ast.hpp` |
| Validation checks | `src/lucene/core/validation/` |
| Services of your own | `src/lucene/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/lucene/cli/main.cpp` |
| LSP features | `src/lucene/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

Lucene's flexible `StandardSyntaxParser` query dialect supports fields, Boolean clauses, ranges and interval expressions.

```
title:"The Right Way" AND text:go
mod_date:[20020101 TO 20030101]
jakarta^4 apache
roam~0.8
(jakarta OR apache) AND website
```

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
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
