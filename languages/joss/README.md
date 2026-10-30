# joss

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
| The grammar | `src/joss/core/JossGrammar.hpp` |
| The AST | `src/joss/core/ast.hpp` |
| Validation checks | `src/joss/core/validation/` |
| Services of your own | `src/joss/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/joss/cli/main.cpp` |
| LSP features | `src/joss/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
Set A=3.
Set B=A+2·3.

1.1 Set A=5.
1.2 Type A.
1.3 Do step 1.1 if A=1.

Type 'hello, world!'.
```

JOSS commands may be direct or stored under a step number. This port models
core forms from RAND’s 1967 JOSS Primer; `*` is exponentiation and the centered
dot `·` is multiplication. Stored expressions, forms and commands have AST nodes.

## The command line

```bash
./build/src/joss_cli tests/statements.joss
./build/src/joss_cli --dump tests/statements.joss   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`joss_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
