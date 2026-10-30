# dice

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
| The grammar | `src/dice/core/DiceGrammar.hpp` |
| The AST | `src/dice/core/ast.hpp` |
| Validation checks | `src/dice/core/validation/` |
| Services of your own | `src/dice/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/dice/cli/main.cpp` |
| LSP features | `src/dice/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
(1d6+2)*3-1d4
```

[Dice notation](https://en.wikipedia.org/wiki/Dice_notation), as used by RPGs
and other tabletop games: `NdM` rolls N dice of M sides each (N may be
omitted, meaning one — `d6` and `1d6` are the same roll); `+ - * /` combine
rolls and plain numbers, `*` and `/` binding tighter than `+` and `-`;
parentheses group. A leading sign belongs to the term it starts, not to a
separate unary operator, so two can stand side by side: `1--1` is `1 - (-1)`.
Space, tab, CR and LF separate tokens. Literal space is an extension of the
archived source lexer, which skipped only tab/CR/LF.

## The command line

```bash
./build/src/dice_cli example/arithmetic_dice.dice
./build/src/dice_cli --dump example/arithmetic_dice.dice   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`dice_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
