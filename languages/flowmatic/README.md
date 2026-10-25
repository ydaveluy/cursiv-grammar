# flowmatic

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
| The grammar | `src/flowmatic/core/FlowmaticGrammar.hpp` |
| The AST | `src/flowmatic/core/ast.hpp` |
| Validation checks | `src/flowmatic/core/validation/` |
| Services of your own | `src/flowmatic/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/flowmatic/cli/main.cpp` |
| LSP features | `src/flowmatic/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
 (0)  INPUT INVENTORY FILE-A PRICE FILE-B ; OUTPUT PRICED-INV FILE-C UNPRICED-INV
     FILE-D ; HSP D .
 (1)  COMPARE PRODUCT-NO (A) WITH PRODUCT-NO (B) ; IF GREATER GO TO OPERATION 10 ;
     IF EQUAL GO TO OPERATION 5 ; OTHERWISE GO TO OPERATION 2 .
     ...
(17)  STOP . (END)
```

[FLOW-MATIC](https://en.wikipedia.org/wiki/FLOW-MATIC) (1958) is a numbered
sequence of lines: each opens on a `(N)` label and holds one or more
statements — `INPUT`/`OUTPUT` file bindings, `COMPARE`/`MOVE`/`TEST` on a
field within a file letter, `IF`/`OTHERWISE ... GO TO OPERATION N` control
transfer — ending on `.`. The program itself ends on the `(END)` marker.

## The command line

```bash
./build/src/flowmatic_cli example/example1.flowmatic
./build/src/flowmatic_cli --dump example/example1.flowmatic   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`flowmatic_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
