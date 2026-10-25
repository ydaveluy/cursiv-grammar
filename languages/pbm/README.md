# pbm

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
| The grammar | `src/pbm/core/PbmGrammar.hpp` |
| The AST | `src/pbm/core/ast.hpp` |
| Validation checks | `src/pbm/core/validation/` |
| Services of your own | `src/pbm/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/pbm/cli/main.cpp` |
| LSP features | `src/pbm/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

A PBM file is a Netpbm plain (ASCII) bitmap: a magic number, the width and
height, then one `0` or `1` per pixel.

```
P1
# This is an example bitmap of the letter "J"
6 10
0 0 0 0 1 0
0 0 0 0 1 0
0 0 0 0 1 0
0 0 0 0 1 0
0 0 0 0 1 0
0 0 0 0 1 0
1 0 0 0 1 0
0 1 1 1 0 0
0 0 0 0 0 0
0 0 0 0 0 0
```

Whitespace between pixels is trivia, not a separator the grammar requires: a
row may just as well be written `000010`.

## The command line

```bash
./build/src/pbm_cli example/example1.pbm
./build/src/pbm_cli --dump example/example1.pbm   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`pbm_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
