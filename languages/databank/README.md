# databank

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
| The grammar | `src/databank/core/DatabankGrammar.hpp` |
| The AST | `src/databank/core/ast.hpp` |
| Validation checks | `src/databank/core/validation/` |
| Services of your own | `src/databank/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/databank/cli/main.cpp` |
| LSP features | `src/databank/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

A [databank](https://en.wikipedia.org/wiki/Databank_format) document is one
NBER-style time series: an optional type code, its two date bounds, then its
readings, one per line, `NA` for a gap:

```
"c  minimal databank example: type code -4, and scientific notation        "
-4
1970.01
1970.12
1.5E+03
2.5e-02
-3.2E-1
NA
```

A quoted `"c ... "` line is a comment. A leading `-1`, `-4` or `-12` marks a
dated series and is followed by its two date bounds; without it, the first
two lines are the bounds of an undated series. Every other line is a sample:
a number, or `NA`.

## The command line

```bash
./build/src/databank_cli example/example1.databank
./build/src/databank_cli --dump example/example1.databank   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`databank_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
