# csv

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
| The grammar | `src/csv/core/CsvGrammar.hpp` |
| The AST | `src/csv/core/ast.hpp` |
| Validation checks | `src/csv/core/validation/` |
| Services of your own | `src/csv/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/csv/cli/main.cpp` |
| LSP features | `src/csv/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
"REVIEW_DATE","AUTHOR","ISBN","DISCOUNTED_PRICE"
"1985/01/21","Douglas Adams",0345391802,5.95
"1998/07/15","Timothy ""The Parser"" Campbell",0968411304,18.99
```

The first row is the header, kept apart from the data rows in `Doc::header`.
A field is `TEXT` (anything but a comma, a quote or a line ending), a quoted
`STRING` (where `""` escapes a literal quote), or empty.

## The command line

```bash
./build/src/csv_cli example/example1.csv
./build/src/csv_cli --dump example/example1.csv   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`csv_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
