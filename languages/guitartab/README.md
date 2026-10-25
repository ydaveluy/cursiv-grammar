# guitartab

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
| The grammar | `src/guitartab/core/GuitartabGrammar.hpp` |
| The AST | `src/guitartab/core/ast.hpp` |
| Validation checks | `src/guitartab/core/validation/` |
| Services of your own | `src/guitartab/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/guitartab/cli/main.cpp` |
| LSP features | `src/guitartab/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
e ---|---|---|---|---|
B -x-|---|---|---|---|
G ---|-x-|---|---|---|
D ---|-x-|---|---|---|
A ---|---|---|---|---|
E ---|---|---|---|---|
```

A tab is one line per guitar string, each starting with the string's note
(`e B G D A E`, upper or lower case). The rest of the line is a run of
`x`/`o`/`-` marks, cut into segments by `|` bars — each segment shows what
happens on that string during one bar.

## The command line

```bash
./build/src/guitartab_cli example/example1.guitartab
./build/src/guitartab_cli --dump example/example1.guitartab   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`guitartab_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
