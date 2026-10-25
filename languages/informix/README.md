# informix

An Informix 4GL language built with [cursiv](https://github.com/ydaveluy/cursiv).
Informix 4GL is a screen-oriented procedural language with embedded SQL:
`MAIN`/`FUNCTION`/`REPORT` blocks, `DEFINE`d variables (scalars, arrays,
records), control flow, and SQL statements (`SELECT`, cursors, `PREPARE`)
written directly inline.

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
| The grammar | `src/informix/core/InformixGrammar.hpp` |
| The AST | `src/informix/core/ast.hpp` |
| Validation checks | `src/informix/core/validation/` |
| Services of your own | `src/informix/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/informix/cli/main.cpp` |
| LSP features | `src/informix/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
MAIN
DEFINE i SMALLINT
FOR i = 1 TO 10
   DISPLAY i
END FOR
END MAIN
```

A `MAIN` block: a `DEFINE`d variable, a `FOR` loop, a `DISPLAY` statement.
`example/` contains sample programs; `tests/coverage.4gl` demonstrates
embedded SQL, cursors, records/arrays, datetime types and screen statements.

## The command line

```bash
./build/src/informix_cli example/example2.4gl
./build/src/informix_cli --dump example/example2.4gl   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`informix_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
