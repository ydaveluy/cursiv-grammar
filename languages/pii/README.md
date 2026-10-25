# pii

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
| The grammar | `src/pii/core/PiiGrammar.hpp` |
| The AST | `src/pii/core/ast.hpp` |
| Validation checks | `src/pii/core/validation/` |
| Services of your own | `src/pii/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/pii/cli/main.cpp` |
| LSP features | `src/pii/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
S0960-9822(11)01319-4
```

A [Publisher Item Identifier](https://en.wikipedia.org/wiki/Publisher_Item_Identifier):
`S` then a compacted 8-digit ISSN (or `B` then a 10-digit ISBN), a 2-digit
year, a 5-digit item number and a check character — a digit, or `X`. `-`,
`(`, `)` and whitespace are all interchangeable separators, ignorable
anywhere between two digits.

## The command line

```bash
./build/src/pii_cli example/example1.pii
./build/src/pii_cli --dump example/example1.pii   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`pii_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
