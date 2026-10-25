# wktcrsv1

WKT CRS v1: a Well-Known Text coordinate reference system definition, and
the GeoTools `epsg.properties` format that maps an EPSG/ESRI code to one per
line.

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
| The grammar | `src/wktcrsv1/core/WktCrsV1Grammar.hpp` |
| The AST | `src/wktcrsv1/core/ast.hpp` |
| Validation checks | `src/wktcrsv1/core/validation/` |
| Services of your own | `src/wktcrsv1/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/wktcrsv1/cli/main.cpp` |
| LSP features | `src/wktcrsv1/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
5705=VERT_CS["Baltic 1977 height", VERT_DATUM["Baltic 1977", 2005, AUTHORITY["EPSG","5105"]], UNIT["m", 1.0], AXIS["Gravity-related height", UP], AUTHORITY["EPSG","5705"]]
```

A row maps an authority code to one WKT CRS definition: `PROJCS`, `GEOGCS`,
`GEOCCS`, `VERT_CS`, `LOCAL_CS` or `COMPD_CS`, each nesting a `DATUM`, a
`SPHEROID`, an optional `TOWGS84`, a `UNIT`, one or more `AXIS` clauses and an
`AUTHORITY`. A `#` line is a comment row, kept in the tree rather than
skipped.

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
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
