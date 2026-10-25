# gvpr

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
| The grammar | `src/gvpr/core/GvprGrammar.hpp` |
| The AST | `src/gvpr/core/ast.hpp` |
| Validation checks | `src/gvpr/core/validation/` |
| Services of your own | `src/gvpr/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/gvpr/cli/main.cpp` |
| LSP features | `src/gvpr/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

[gvpr](https://graphviz.org/pdf/gvpr.1.pdf) is Graphviz's graph-pattern
scanning language: a `BEGIN`/`N`/`E`/`END`-shaped script with C-flavoured
statements and expressions, run over one or more graphs.

```
N[layer=="(*a*)"]
```

`t1.gvpr`: a bare pattern with no action -- every node whose `layer`
attribute is `"(*a*)"` matches. A fuller shape:

```
BEG_G {
  node_t n = isNode($, ARGV[0]);
}
N [(indegree == 1) && (outdegree == 1)] {
  color = "blue";
}
```

`BEG_G`/`N`/`E`/`END_G` blocks run once per graph, once per node, once per
edge; `N`/`E` additionally take a `[...]` guard evaluated before the `{...}`
action runs.

## The command line

```bash
./build/src/gvpr_cli example/t1.gvpr
./build/src/gvpr_cli --dump example/t1.gvpr   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`gvpr_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
