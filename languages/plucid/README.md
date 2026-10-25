# plucid

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
| The grammar | `src/plucid/core/PlucidGrammar.hpp` |
| The AST | `src/plucid/core/ast.hpp` |
| Validation checks | `src/plucid/core/validation/` |
| Services of your own | `src/plucid/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/plucid/cli/main.cpp` |
| LSP features | `src/plucid/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

A plucid document is a single expression -- [Lucid](https://en.wikipedia.org/wiki/Lucid_(programming_language))'s
dataflow language, filtered to the subset its reference grammar defines:

```
if x eq 0 then error else 1 / x fi
```

`error` and `eod` are the language's two sentinel values; `filter`, `substr`,
`length` and `arg` are built-in unary/ternary forms; `if`/`case`/`cond` are
its three branching expressions. `example/` holds three documents, each a
`[% ... %]` list packing many small expressions into the one the grammar
allows at the top: `arithmetic.plu` (numbers and numeric operators),
`text.plu` (quoted words, strings, lists) and `flow.plu` (control forms,
the remaining operators, calls).

## The command line

```bash
./build/src/plucid_cli tests/arithmetic.plu
./build/src/plucid_cli --dump tests/arithmetic.plu   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`plucid_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
