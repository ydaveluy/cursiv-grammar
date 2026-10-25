# wln

A language library built with [cursiv](https://github.com/ydaveluy/cursiv).

## Build

```bash
cmake -B build
cmake --build build -j
ctest --test-dir build --output-on-failure
```

The build generates the parser automatically. Rebuild after changing the grammar.

## What to edit

| What you want to change | File |
|---|---|
| The grammar | `src/wln/core/WlnGrammar.hpp` |
| The AST | `src/wln/core/ast.hpp` |
| Validation checks | `src/wln/core/validation/` |
| Services of your own | `src/wln/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/wln/cli/main.cpp` |
| LSP features | `src/wln/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

[WLN](https://en.wikipedia.org/wiki/Wiswesser_line_notation) writes a
molecule as a line of single-character symbols, each one an atom, a bond or
a ring feature:

```
QVYZ1R
```

Phenylalanine: `Q` hydroxyl, `V` carbonyl, `Y` a branching carbon, `Z` amino,
`1` a chain of one carbon, `R` a benzene ring. Several such groups, separated
by one space, describe a mixture:

```
1H Q1
```

## The command line

```bash
./build/src/wln_cli example/phenylalanine.wln
./build/src/wln_cli --dump example/phenylalanine.wln   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`wln_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
