# fdo91

A language library built with [cursiv](https://github.com/ydaveluy/cursiv).

The target is the textual atom notation of historical FDO91 examples, not the
binary FDO wire protocol. The primary reference available for this review was
[AOL's FDO91 Manual, volume 2, chapter 15](https://mail.koin.org/files/aol.aim/aol/fdo/manuals/de.pdf),
January 1994. It establishes `atom$` names and command-specific operands, but
is not the complete lexical/compiler specification; the full manual archive
was inaccessible. No exact compiler edition is pinned by this corpus.

Nested atoms, global IDs and raw numeric/hex spellings are represented; strings
are partially decoded. The port's signed-64 range and piped-argument checks are
local policies, not verified universal native constraints. Command signatures,
protocol data types, escape semantics and complete compiler acceptance remain
unverified. No original AOL compiler or runtime was executed.

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
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
