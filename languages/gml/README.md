# gml

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
| The grammar | `src/gml/core/GmlGrammar.hpp` |
| The AST | `src/gml/core/ast.hpp` |
| Validation checks | `src/gml/core/validation/` |
| Services of your own | `src/gml/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/gml/cli/main.cpp` |
| LSP features | `src/gml/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

[GML](https://en.wikipedia.org/wiki/Graph_Modelling_Language) (Graph
Modelling Language) is a plain-text format for attributed graphs: a
document is a run of `key value` pairs, and a value can itself be a
bracketed run of pairs, nested to any depth.

```
graph
[
  directed 1
  label "a tiny graph"
  node [ id 1 label "A" ]
  node [ id 2 label "B" ]
  edge [ source 1 target 2 label "A to B" ]
]
```

`graph`, `node`, `edge`, `id`, `label`, `source` and `target` are not
keywords -- the grammar is fully generic, a key is any bare word and a
value is a number, a string, a bare word, or a `[ ... ]` list. The
convention above is just the most common one.

## The command line

```bash
./build/src/gml_cli example/karate.gml
./build/src/gml_cli --dump example/karate.gml   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`gml_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
