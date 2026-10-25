# fdo91

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
| The grammar | `src/fdo91/core/Fdo91Grammar.hpp` |
| The AST | `src/fdo91/core/ast.hpp` |
| Validation checks | `src/fdo91/core/validation/` |
| Services of your own | `src/fdo91/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/fdo91/cli/main.cpp` |
| LSP features | `src/fdo91/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

fdo91 is AOL's FDO91 "atom" script format: a flat stream of commands, each
optionally followed by its arguments -- a bare run of literals, or a
`<...>` list.

```
uni_start_stream
if_online_then <1, 2>
async_alert <info, "You are online">
uni_sync_skip <1>
async_alert <info, "You are NOT online">
uni_sync_skip <2>
uni_end_stream
```

An argument can itself be a run of nested atoms sharing one `<...>` slot, with
no separator between them -- `act_replace_select_action`'s single argument in
`example/example7.fdo91` is three atoms deep.

## The command line

```bash
./build/src/fdo91_cli example/example6.fdo91
./build/src/fdo91_cli --dump example/example6.fdo91   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`fdo91_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
