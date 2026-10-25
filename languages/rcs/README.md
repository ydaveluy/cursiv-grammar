# rcs

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
| The grammar | `src/rcs/core/RcsGrammar.hpp` |
| The AST | `src/rcs/core/ast.hpp` |
| Validation checks | `src/rcs/core/validation/` |
| Services of your own | `src/rcs/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/rcs/cli/main.cpp` |
| LSP features | `src/rcs/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

RCS is the file format behind the Revision Control System's `,v` files: an
admin block, then one delta per revision (its date, author, state and links
to neighbouring revisions), then the description, then one deltatext per
revision (its log message and full text).

```
head	1.1;
access;
symbols;
desc
@Minimal admin section: only the fields the grammar requires, no deltas.@
```

A full file adds `branch`, `locks`, `strict`, `comment`, `expand`, symbolic
tags (`release-1_0:1.1`) and deltas with `branches`/`next` links — see
`example/full.rcs`. A value that must hold a literal `@` doubles it
(`@a @@ b@` reads as `a @ b`), the same escaping SQL uses for `'`.

## The command line

```bash
./build/src/rcs_cli tests/full.rcs
./build/src/rcs_cli --dump tests/full.rcs   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`rcs_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
