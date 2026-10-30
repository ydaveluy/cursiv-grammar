# clif

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
| The grammar | `src/clif/core/ClifGrammar.hpp` |
| The AST | `src/clif/core/ast.hpp` |
| Validation checks | `src/clif/core/validation/` |
| Services of your own | `src/clif/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/clif/cli/main.cpp` |
| LSP features | `src/clif/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

CLIF is an s-expression syntax for Common Logic texts, with modules and
first-order sentences. This port is based on ISO/IEC 24707:2007, Annex A.

```
(cl-module ring (cl-imports http://colore.oor.net/ringoids/ring.clif))

(forall (x)
        (if     (magma x)
                (not (ring x))))
```

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
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
