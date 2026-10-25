# gtin

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
| The grammar | `src/gtin/core/GtinGrammar.hpp` |
| The AST | `src/gtin/core/ast.hpp` |
| Validation checks | `src/gtin/core/validation/` |
| Services of your own | `src/gtin/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/gtin/cli/main.cpp` |
| LSP features | `src/gtin/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
5901234123457
```

A GTIN is read digit by digit, no separators required — hyphens and
whitespace are trivia, so `590-1234-12345-7` reads the same. `example/`
holds one document per shape the grammar accepts: an EAN-8, a UPC-A and
UPC-E, all four EAN-13 forms (generic, ISMN, ISSN, Bookland), an EAN-14, and
the 2- and 5-digit supplemental codes.

## The command line

```bash
./build/src/gtin_cli example/gtin13.gtin
./build/src/gtin_cli --dump example/gtin13.gtin   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`gtin_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
