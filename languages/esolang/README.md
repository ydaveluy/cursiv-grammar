# lolcode

A language library built with [cursiv](https://github.com/ydaveluy/cursiv).

This library parses LOLCODE 1.2 programs and the corpus compatibility spellings shown below.

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
| The grammar | `src/lolcode/core/LolcodeGrammar.hpp` |
| The AST | `src/lolcode/core/ast.hpp` |
| Validation checks | `src/lolcode/core/validation/` |
| Services of your own | `src/lolcode/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/lolcode/cli/main.cpp` |
| LSP features | `src/lolcode/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
HAI 1.3
VISIBLE "Hai world!"
KTHXBYE
```

`HAI` opens a program and `KTHXBYE` closes it. `VISIBLE` prints its arguments; programs can also declare variables, functions, loops and conditionals.

## The command line

```bash
./build/src/lolcode_cli example/hai.lol
./build/src/lolcode_cli --dump example/hai.lol   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`lolcode_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
