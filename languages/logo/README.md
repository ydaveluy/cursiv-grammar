# logo

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
| The grammar | `src/logo/core/LogoGrammar.hpp` |
| The AST | `src/logo/core/ast.hpp` |
| Validation checks | `src/logo/core/validation/` |
| Services of your own | `src/logo/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/logo/cli/main.cpp` |
| LSP features | `src/logo/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
repeat 3 [fd 60 rt 120]
```

`repeat` runs a bracketed block of commands a number of times; `fd`/`rt`
move and turn a turtle. `make "name value` sets a variable, `:name` reads
it, and `to name :param ... end` declares a procedure. See `example/` for
more.

## The command line

```bash
./build/src/logo_cli example/repeat.logo
./build/src/logo_cli --dump example/repeat.logo   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`logo_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
