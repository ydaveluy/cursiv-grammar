# recfile

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
| The grammar | `src/recfile/core/RecfileGrammar.hpp` |
| The AST | `src/recfile/core/ast.hpp` |
| Validation checks | `src/recfile/core/validation/` |
| Services of your own | `src/recfile/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/recfile/cli/main.cpp` |
| LSP features | `src/recfile/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
%rec: Texts
%type: Year int

Author: Doug McIlroy
Year: 1964
Note: The Origin of Unix Pipes
```

A `%key: value` line is a descriptor — metadata about the records that
follow, such as `%rec` naming the record set. Any other `key: value` line is
a field. A value may continue onto the next physical line when that line
opens with `+`.

## The command line

```bash
./build/src/recfile_cli example/example1.rec
./build/src/recfile_cli --dump example/example1.rec   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`recfile_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
