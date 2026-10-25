# clif

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
| The grammar | `src/clif/core/ClifGrammar.hpp` |
| The AST | `src/clif/core/ast.hpp` |
| Validation checks | `src/clif/core/validation/` |
| Services of your own | `src/clif/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/clif/cli/main.cpp` |
| LSP features | `src/clif/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

CLIF (Common Logic Interchange Format, ISO/IEC 24707) is an s-expression
syntax for first-order logic texts:

```
(cl-module ring (cl-imports http://colore.oor.net/ringoids/ring.clif))

(forall (x)
        (if     (magma x)
                (not (ring x))))
```

A document is a sequence of modules, named texts (`cl-text`) and bare
sentences. A sentence is an equation, a predication, a boolean combination
(`and`/`or`/`if`/`iff`/`not`), a quantified sentence (`forall`/`exists` over
a list of bound names), or a commented sentence (`cl-comment`).

## The command line

```bash
./build/src/clif_cli example/module.clif
./build/src/clif_cli --dump example/module.clif   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`clif_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
