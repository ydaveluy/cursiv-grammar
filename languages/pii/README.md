# pii

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
| The grammar | `src/pii/core/PiiGrammar.hpp` |
| The AST | `src/pii/core/ast.hpp` |
| Validation checks | `src/pii/core/validation/` |
| Services of your own | `src/pii/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/pii/cli/main.cpp` |
| LSP features | `src/pii/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

Publisher Item Identifiers for journals and books, with ISSN or ISBN components.

```
S0960-9822(11)01319-4
```

```
S0012365X15000898
S9999-9994(03)X7607-2
B978-0-323-01679-7.10003-8
B0-323-01679-0/10027-7
```

## The command line

```bash
./build/src/pii_cli example/example1.pii
./build/src/pii_cli --dump example/example1.pii   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`pii_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
