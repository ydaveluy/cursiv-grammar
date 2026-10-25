# bibcode

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
| The grammar | `src/bibcode/core/BibcodeGrammar.hpp` |
| The AST | `src/bibcode/core/ast.hpp` |
| Validation checks | `src/bibcode/core/validation/` |
| Services of your own | `src/bibcode/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/bibcode/cli/main.cpp` |
| LSP features | `src/bibcode/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
1974AJ.....79..819H
```

A [Bibcode](https://en.wikipedia.org/wiki/Bibcode): the 19-character
identifier astronomical databases (ADS, SIMBAD) use to cite a publication —
year (4 digits), journal (5 letters/digits/dots), volume (4), an optional
qualifying letter, the page (one or more digits/dots), and the first letter
of the first author's last name.

## The command line

```bash
./build/src/bibcode_cli example/example1.bibcode
./build/src/bibcode_cli --dump example/example1.bibcode   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`bibcode_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
