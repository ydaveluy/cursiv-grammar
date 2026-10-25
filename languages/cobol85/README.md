# cobol85

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
| The grammar | `src/cobol85/core/Cobol85Grammar.hpp` |
| The AST | `src/cobol85/core/ast.hpp` |
| Validation checks | `src/cobol85/core/validation/` |
| Services of your own | `src/cobol85/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/cobol85/cli/main.cpp` |
| LSP features | `src/cobol85/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```cobol
IDENTIFICATION DIVISION.
PROGRAM-ID. HelloWorld.

PROCEDURE DIVISION.
DISPLAY "Hello World".
STOP RUN.
```

Free-format COBOL 85: four divisions (IDENTIFICATION, ENVIRONMENT, DATA,
PROCEDURE), a flat list of level-numbered data description entries under
DATA DIVISION, and paragraphs of statements under PROCEDURE DIVISION. See
`tests/` for DATA DIVISION clauses (PICTURE, OCCURS, REDEFINES, level 88
condition names, indexed tables) and common PROCEDURE DIVISION statements
(arithmetic, IF/EVALUATE, PERFORM, STRING, SEARCH, file I/O).

## The command line

```bash
./build/src/cobol85_cli example/example1.cbl
./build/src/cobol85_cli --dump example/example1.cbl   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`cobol85_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
