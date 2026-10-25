# sapf

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
| The grammar | `src/sapf/core/SapfGrammar.hpp` |
| The AST | `src/sapf/core/ast.hpp` |
| Validation checks | `src/sapf/core/validation/` |
| Services of your own | `src/sapf/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/sapf/cli/main.cpp` |
| LSP features | `src/sapf/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

[sapf](https://github.com/lfnoise/sapf) is concatenative: a document is a flat
sequence of numbers, words, lists, forms and functions that a stack-based
interpreter folds left to right.

```
{analog_bubbles :rate1 -.2 :decay 8}.out play  ; change parameters
```

`{...}` is a form (`:rate1`/`:decay` its keys), `.out` a word prefixed by `.`,
and `analog_bubbles`/`play` plain words — every atom stands for a stack
operation the real interpreter carries out; the grammar only fixes their
shape.

## The command line

```bash
./build/src/sapf_cli example/example28.sapf
./build/src/sapf_cli --dump example/example28.sapf   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`sapf_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
