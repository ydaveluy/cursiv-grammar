# fen

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
| The grammar | `src/fen/core/FenGrammar.hpp` |
| The AST | `src/fen/core/ast.hpp` |
| Validation checks | `src/fen/core/validation/` |
| Services of your own | `src/fen/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/fen/cli/main.cpp` |
| LSP features | `src/fen/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

[FEN](https://en.wikipedia.org/wiki/Forsyth%E2%80%93Edwards_Notation) (Forsyth–Edwards
Notation) records one chess position on one line: the board rank by rank,
side to move, castling rights, an en passant target square, and the two move
clocks.

```
rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1
```

Each of the eight `/`-separated ranks spells its squares left to right: a
letter for a piece (uppercase white, lowercase black) or a digit for a run of
empty squares. `b` is the side to move, `KQkq` which castling rights remain,
`e3` the en passant target (`-` when there is none), and `0 1` the halfmove
and fullmove clocks.

## The command line

```bash
./build/src/fen_cli example/example1.fen
./build/src/fen_cli --dump example/example1.fen   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`fen_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
