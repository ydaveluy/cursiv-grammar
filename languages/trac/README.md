# trac

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
| The grammar | `src/trac/core/TracGrammar.hpp` |
| The AST | `src/trac/core/ast.hpp` |
| Validation checks | `src/trac/core/validation/` |
| Services of your own | `src/trac/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/trac/cli/main.cpp` |
| LSP features | `src/trac/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
#(read string,handle,default)
```

[TRAC](https://en.wikipedia.org/wiki/TRAC_(programming_language)) is a
macro-processing language: a document is a sequence of macro calls. `#(...)`
is an active call, `##(...)` a neutral one; the first argument is the macro's
name, the rest its arguments — a bare macro, a parenthesized one, or plain
text. An argument's text may carry embedded spaces ("read string" above is
the name, one argument), so the AST holds a `string_view` per argument rather
than a list of words.

## The command line

```bash
./build/src/trac_cli example/example1.trac
./build/src/trac_cli --dump example/example1.trac   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`trac_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
