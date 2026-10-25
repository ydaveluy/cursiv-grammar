# d2

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
| The grammar | `src/d2/core/D2Grammar.hpp` |
| The AST | `src/d2/core/ast.hpp` |
| Validation checks | `src/d2/core/validation/` |
| Services of your own | `src/d2/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/d2/cli/main.cpp` |
| LSP features | `src/d2/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
pg: PostgreSQL
Cloud: my cloud
Cloud.shape: cloud

pg -> Cloud: reads from
```

A node is a dotted name, an optional `: label`, an optional `[...]`
attribute list and an optional `{...}` block of nested statements. An edge
chains two or more names with `->`, `<-` or `--`, and takes the same
optional label and attributes. See `example/` for the constructs this
grammar accepts.

## The command line

```bash
./build/src/d2_cli example/example2.d2
./build/src/d2_cli --dump example/example2.d2   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`d2_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
