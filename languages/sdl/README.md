# sdl

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
| The grammar | `src/sdl/core/SdlGrammar.hpp` |
| The AST | `src/sdl/core/ast.hpp` |
| Validation checks | `src/sdl/core/validation/` |
| Services of your own | `src/sdl/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/sdl/cli/main.cpp` |
| LSP features | `src/sdl/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

SDL (the HP/DEC Structure Definition Language) describes a data structure once
and translates it into several target languages. A module holds constants,
items and aggregates:

```
MODULE example;

CONSTANT max_entries EQUALS 10;

AGGREGATE record STRUCTURE PREFIX "rec_";
    id        LONGWORD;
    flags     BITFIELD MASK LENGTH 4;
    entries   WORD DIMENSION 0:max_entries-1;
END record;

ITEM record_count ADDRESS GLOBAL;

END_MODULE example;
```

`AGGREGATE`/`END` declares a `STRUCTURE` or `UNION`; `ITEM` and `CONSTANT`
declare a single field or value. `example/` contains sample SDL documents.

## The command line

```bash
./build/src/sdl_cli example/test1.sdl
./build/src/sdl_cli --dump example/test1.sdl   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`sdl_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
