# haskell

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
| The grammar | `src/haskell/core/HaskellGrammar.hpp` |
| The AST | `src/haskell/core/ast.hpp` |
| Validation checks | `src/haskell/core/validation/` |
| Services of your own | `src/haskell/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/haskell/cli/main.cpp` |
| LSP features | `src/haskell/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```haskell
data Tree a = Leaf a | Branch (Tree a) (Tree a)

depth :: Tree a -> Int
depth (Leaf _) = 1
depth (Branch l r) = 1 + max (depth l) (depth r)
```

A suite (`where`/`let`/`do`/`of`) opens on an indented line, the way
`../python` does; nested comments (`{- a {- b -} c -}`) balance. A
declaration's right-hand side may not continue onto a further, unbracketed
line.

## The command line

```bash
./build/src/haskell_cli example/tree.hs
./build/src/haskell_cli --dump example/tree.hs   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`haskell_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
