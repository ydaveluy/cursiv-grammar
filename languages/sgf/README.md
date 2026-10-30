# sgf

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
| The grammar | `src/sgf/core/SgfGrammar.hpp` |
| The AST | `src/sgf/core/ast.hpp` |
| Validation checks | `src/sgf/core/validation/` |
| Services of your own | `src/sgf/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/sgf/cli/main.cpp` |
| LSP features | `src/sgf/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

SGF FF[4] game trees with ordered nodes and properties. Values retain source escapes; game-specific rules are outside this parser.

```
(;FF[4]GM[1]SZ[19]C[a comment];B[aa](;W[bb])(;W[cc]))
```

## The command line

```bash
./build/src/sgf_cli example/8718-KuroNeko-creedfoxx-chensicha.sgf
./build/src/sgf_cli --dump example/8718-KuroNeko-creedfoxx-chensicha.sgf   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`sgf_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
