# basic

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
| The grammar | `src/basic/core/BasicGrammar.hpp` |
| The AST | `src/basic/core/ast.hpp` |
| Validation checks | `src/basic/core/validation/` |
| Services of your own | `src/basic/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/basic/cli/main.cpp` |
| LSP features | `src/basic/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```basic
100 PRINT "Hello world"
```

jvmBASIC ([source](https://github.com/teverett/jvmBASIC)) is a line-numbered
BASIC: every line starts with a `NUMBER`, and `GOTO`/`GOSUB` jump to one --
there is no symbol table, so nothing here is a cross reference. Whitespace,
newlines included, is insignificant: a line boundary is wherever the next
line number can start, not a token of its own.

```basic
100 FOR I = 1 TO 10
110   PRINT I; " squared is "; I * I
120 NEXT I
130 IF I > 5 THEN GOTO 200
200 END
```

## The command line

```bash
./build/src/basic_cli example/hello.bas
./build/src/basic_cli --dump example/hello.bas   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`basic_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
