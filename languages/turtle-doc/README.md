# turtledoc

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
| The grammar | `src/core/turtledoc/TurtleDocGrammar.hpp` |
| The AST | `src/core/turtledoc/ast.hpp` |
| Validation checks | `src/core/turtledoc/validation/` |
| Services of your own | `src/core/turtledoc/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/cli.cpp` |
| LSP features | `src/lsp/turtledoc/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

An independent RDF 1.1 Turtle parser, preserving declarations and triples in source order.

```turtle
@prefix ex: <http://example.org/> .
ex:Alice a ex:Person ; ex:name "Alice"@en . # a comment
```

## The command line

```bash
./build/src/turtledoc_cli tests/features.ttl
./build/src/turtledoc_cli --dump tests/features.ttl   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`turtledoc_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.91 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
