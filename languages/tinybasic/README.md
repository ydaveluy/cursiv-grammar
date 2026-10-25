# tinybasic

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
| The grammar | `src/tinybasic/core/TinybasicGrammar.hpp` |
| The AST | `src/tinybasic/core/ast.hpp` |
| Validation checks | `src/tinybasic/core/validation/` |
| Services of your own | `src/tinybasic/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/tinybasic/cli/main.cpp` |
| LSP features | `src/tinybasic/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
50 INPUT "GUESS A NUMBER?", G
60 C = C+1
70 IF G=N GOTO 110
80 IF G>N PRINT "LOWER"
90 IF G<N PRINT "HIGHER"
100 GOTO 50
110 PRINT "YOU GUESSED IT IN", C, " TRIES!"
```

[TinyBASIC](https://en.wikipedia.org/wiki/Tiny_BASIC): each line optionally
starts with a line number, and holds one statement -- `PRINT`, `IF`, `GOTO`,
`INPUT`, an assignment (`LET` is itself optional), `GOSUB`/`RETURN`, or one of
`CLEAR`/`LIST`/`RUN`/`END`. A variable is a single letter `A`-`Z`; arithmetic
is `+ - * /` over variables and numbers, with `IF` comparing two expressions
with `< <= <> >< > >= =`.

## The command line

```bash
./build/src/tinybasic_cli example/example1.tinybasic
./build/src/tinybasic_cli --dump example/example1.tinybasic   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`tinybasic_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
