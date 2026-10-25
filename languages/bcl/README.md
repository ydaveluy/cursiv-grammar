# bcl

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
| The grammar | `src/bcl/core/BclGrammar.hpp` |
| The AST | `src/bcl/core/ast.hpp` |
| Validation checks | `src/bcl/core/validation/` |
| Services of your own | `src/bcl/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/bcl/cli/main.cpp` |
| LSP features | `src/bcl/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
11010000
```

[Binary combinatory logic](https://en.wikipedia.org/wiki/Binary_combinatory_logic)
encodes an SK-term as a bitstring: `00` is the `K` combinator, `01` is `S`,
and `1` prefixes an application of the term that follows to the term after
it. The line above is `((S K) K)`, the identity combinator, read left to
right. `example/` holds more: several terms in one file, terms written with
no separator between them, and the empty document (zero terms is valid).

## The command line

```bash
./build/src/bcl_cli tests/program.bcl
./build/src/bcl_cli --dump tests/program.bcl   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`bcl_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
