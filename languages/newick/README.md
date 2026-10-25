# newick

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
| The grammar | `src/newick/core/NewickGrammar.hpp` |
| The AST | `src/newick/core/ast.hpp` |
| Validation checks | `src/newick/core/validation/` |
| Services of your own | `src/newick/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/newick/cli/main.cpp` |
| LSP features | `src/newick/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

[Newick](https://en.wikipedia.org/wiki/Newick_format) writes a phylogenetic
tree as nested, comma-separated parentheses, terminated by `;`:

```
(A:0.1,B:0.2,(C:0.3,D:0.4)E:0.5)F;
```

Each node is a *clade*: an optional label, an optional `:` branch length,
and — between parentheses — its children. A clade with no label and no
length is still a real, anonymous node, so `(,,(,));` is a tree of five
unnamed clades.

## The command line

```bash
./build/src/newick_cli example/example7.nwk
./build/src/newick_cli --dump example/example7.nwk   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`newick_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
