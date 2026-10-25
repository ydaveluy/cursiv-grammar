# wavefront

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
| The grammar | `src/wavefront/core/WavefrontGrammar.hpp` |
| The AST | `src/wavefront/core/ast.hpp` |
| Validation checks | `src/wavefront/core/validation/` |
| Services of your own | `src/wavefront/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/wavefront/cli/main.cpp` |
| LSP features | `src/wavefront/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
# a unit square
v 0.0 0.0 0.0
v 1.0 0.0 0.0
v 1.0 1.0 0.0
v 0.0 1.0 0.0

g square
f 1 2 3 4
```

Each line is one directive: `v` a geometric vertex, `g` the group the
elements that follow belong to, `f` a face by its vertices' 1-based indices
(negative indices count back from the end). There is no cross reference to
resolve — an index is a plain number, not a name a linker looks up.

## The command line

```bash
./build/src/wavefront_cli example/b1_20_cube.obj
./build/src/wavefront_cli --dump example/b1_20_cube.obj   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`wavefront_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
