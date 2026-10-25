# sgf

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
| The grammar | `src/sgf/core/SgfGrammar.hpp` |
| The AST | `src/sgf/core/ast.hpp` |
| Validation checks | `src/sgf/core/validation/` |
| Services of your own | `src/sgf/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/sgf/cli/main.cpp` |
| LSP features | `src/sgf/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
(;FF[4]GM[1]SZ[19]ST[2]CA[UTF-8]AP[SGFC:1.17]
PB[chensicha]BR[14k]HA[2]PW[creedfoxx]WR[13k]KM[0.5]
AB[pd][dp]C[a comment];W[dd];B[pp]
(;W[nc])
(;W[qf]))
```

A collection is one or more game trees; a game tree is a sequence of nodes
(`;` followed by properties) and its variations, nested in parentheses. A
property is an uppercase identifier followed by one or more bracketed values
— `AB[pd][dp]` sets up two black stones, `C[...]` attaches a comment. SGF
(Smart Game Format, FF[4]) is a general record format for board games; this
grammar accepts the ~70 standard property identifiers and any other run of
two or more uppercase letters as a private property.

## The command line

```bash
./build/src/sgf_cli example/8718-KuroNeko-creedfoxx-chensicha.sgf
./build/src/sgf_cli --dump example/8718-KuroNeko-creedfoxx-chensicha.sgf   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`sgf_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
