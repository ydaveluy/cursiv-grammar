# wktcrsv1

WKT CRS v1: a Well-Known Text coordinate reference system definition, and
the GeoTools `epsg.properties` format that maps an EPSG/ESRI code to one per
line.

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
| The grammar | `src/wktcrsv1/core/WktCrsV1Grammar.hpp` |
| The AST | `src/wktcrsv1/core/ast.hpp` |
| Validation checks | `src/wktcrsv1/core/validation/` |
| Services of your own | `src/wktcrsv1/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/wktcrsv1/cli/main.cpp` |
| LSP features | `src/wktcrsv1/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

WKT1 coordinate-reference-system descriptions, including OGC and ESRI variants.

```
5705=VERT_CS["Baltic 1977 height",VERT_DATUM["Baltic 1977",2005],UNIT["m",1],AXIS["height",UP]]
```

## The command line

```bash
./build/src/wktcrsv1_cli example/example1.wktcrsv1
./build/src/wktcrsv1_cli --dump example/example1.wktcrsv1   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`wktcrsv1_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
