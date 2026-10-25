# gff3

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
| The grammar | `src/gff3/core/Gff3Grammar.hpp` |
| The AST | `src/gff3/core/ast.hpp` |
| Validation checks | `src/gff3/core/validation/` |
| Services of your own | `src/gff3/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/gff3/cli/main.cpp` |
| LSP features | `src/gff3/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
##gff-version 3
#!gff-spec-version 1.20
NC_001792.2	RefSeq	region	1	1758	.	+	.	ID=id0;gbkey=Src;genome=genomic
NC_001792.2	RefSeq	gene	55	993	.	+	.	ID=gene0;Name=V1;Parent=id0
```

A document is a fixed version header, then a line at a time: a `#` comment
kept verbatim, or a tab-separated feature with nine columns and a `;`-list of
`key=value` attributes. Every field is text — GFF3 spells a missing value as
a literal `.`, so `start`, `end`, `score`, `strand` and `phase` stay
`string_view` rather than a typed number that `.` could not hold.

## The command line

```bash
./build/src/gff3_cli example/NC_001792.gff
./build/src/gff3_cli --dump example/NC_001792.gff   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`gff3_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
