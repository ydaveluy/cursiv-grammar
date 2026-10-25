# pgn

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
| The grammar | `src/pgn/core/PgnGrammar.hpp` |
| The AST | `src/pgn/core/ast.hpp` |
| Validation checks | `src/pgn/core/validation/` |
| Services of your own | `src/pgn/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/pgn/cli/main.cpp` |
| LSP features | `src/pgn/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
[Event "F/S Return Match"]
[Site "Belgrade, Serbia JUG"]
[Date "1992.11.04"]
[Round "29"]
[White "Fischer, Robert J."]
[Black "Spassky, Boris V."]
[Result "1-0"]

1.e4 e5 2.Nf3 Nc6 3.Bb5 a6 4.Ba4 Nf6 5.O-O Be7 ... 39.Rxb6 1-0
```

A PGN document is a database of games, each a tag section (`[Name "Value"]`
pairs) followed by movetext: move numbers, SAN moves, numeric annotation
glyphs (`$1`) and parenthesised variations, ending in a result marker.

## The command line

```bash
./build/src/pgn_cli tests/game.pgn
./build/src/pgn_cli --dump tests/game.pgn   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`pgn_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
