# csv

A language library built with [cursiv](https://github.com/ydaveluy/cursiv).

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
| The grammar | `src/csv/core/CsvGrammar.hpp` |
| The AST | `src/csv/core/ast.hpp` |
| Validation checks | `src/csv/core/validation/` |
| Services of your own | `src/csv/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/csv/cli/main.cpp` |
| LSP features | `src/csv/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

Rows and fields retain their order, including the first row and empty fields. This permissive CSV dialect accepts CR, LF and CRLF record endings and doubled quotes in quoted fields.

```
"REVIEW_DATE","AUTHOR","ISBN","DISCOUNTED_PRICE"
"1985/01/21","Douglas Adams",0345391802,5.95
"1998/07/15","Timothy ""The Parser"" Campbell",0968411304,18.99
```

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
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
