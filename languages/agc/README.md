# agc

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
| The grammar | `src/agc/core/AgcGrammar.hpp` |
| The AST | `src/agc/core/ast.hpp` |
| Validation checks | `src/agc/core/validation/` |
| Services of your own | `src/agc/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/agc/cli/main.cpp` |
| LSP features | `src/agc/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

AGC assembly source for the Apollo Guidance Computer (Apollo 4, Solarium
055). Whitespace is not trivia here: it is the separator between a line's
label, opcode and operand fields, so the grammar reads it explicitly.

```
BEGINNER	TC	BANKCALL
CADRMODE	CADR	IMUREENT

STARTDT1	DEC	200
```

A label starts a line at column 0; an opcode and its operands are indented.
`example/` contains `.agc` documents from real Apollo flight software
listings.

## The command line

```bash
./build/src/agc_cli example/DUMMY_501_INITIALIZATION.agc
./build/src/agc_cli --dump example/DUMMY_501_INITIALIZATION.agc   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`agc_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
