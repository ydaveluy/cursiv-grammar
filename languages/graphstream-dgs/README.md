# graphstreamdgs

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
| The grammar | `src/graphstreamdgs/core/GraphstreamDgsGrammar.hpp` |
| The AST | `src/graphstreamdgs/core/ast.hpp` |
| Validation checks | `src/graphstreamdgs/core/validation/` |
| Services of your own | `src/graphstreamdgs/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/graphstreamdgs/cli/main.cpp` |
| LSP features | `src/graphstreamdgs/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
DGS004
triangle 0 6
an A
an B
an C
ae AB A B
ae BC B C
ae CA C A
```

[GraphStream DGS](http://graphstream-project.org/doc/Advanced-Concepts/The-DGS-File-Format/)
is the event log a GraphStream graph reads or writes: a header naming the
format version and the graph, then a stream of `an`/`cn`/`dn` (node),
`ae`/`ce`/`de` (edge) and `cg`/`st`/`cl` (graph, time step, clear) events,
each carrying `name:value` attributes.

## The command line

```bash
./build/src/graphstreamdgs_cli example/triangle1.dgs
./build/src/graphstreamdgs_cli --dump example/triangle1.dgs   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`graphstreamdgs_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
