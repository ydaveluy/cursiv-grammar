# stl

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
| The grammar | `src/stl/core/StlGrammar.hpp` |
| The AST | `src/stl/core/ast.hpp` |
| Validation checks | `src/stl/core/validation/` |
| Services of your own | `src/stl/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/stl/cli/main.cpp` |
| LSP features | `src/stl/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
solid triangle
  facet normal 0.0 0.0 1.0
    outer loop
      vertex 0.0 0.0 0.0
      vertex 1.0 0.0 0.0
      vertex 0.0 1.0 0.0
    endloop
  endfacet
endsolid triangle
```

An ASCII STL solid: an optional name, zero or more facets (each a normal and
exactly three vertices, all triples of floats), and a closing name. More
documents live under `example/`, one per construct — an anonymous solid, a
multi-facet one, an empty one, and every spelling `FLOAT` accepts.

## The command line

```bash
./build/src/stl_cli tests/triangle.stl
./build/src/stl_cli --dump tests/triangle.stl   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`stl_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
