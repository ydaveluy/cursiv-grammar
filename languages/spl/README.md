# spl

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
| The grammar | `src/spl/core/SplGrammar.hpp` |
| The AST | `src/spl/core/ast.hpp` |
| Validation checks | `src/spl/core/validation/` |
| Services of your own | `src/spl/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/spl/cli/main.cpp` |
| LSP features | `src/spl/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
index=web sourcetype=access_combined
  | rex field=_raw "user=(?<user>\w+)"
  | table _time user status
```

A search: a leading, comma-less list of terms (here two field comparisons,
ANDed by mere juxtaposition), piped into `rex` and `table`. `search` may be
written or dropped; `|` starts every stage after the first.

## The command line

```bash
./build/src/spl_cli example/simple_query.spl
./build/src/spl_cli --dump example/simple_query.spl   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`spl_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
