# bdf

A [Glyph Bitmap Distribution Format](https://en.wikipedia.org/wiki/Glyph_Bitmap_Distribution_Format)
font parser, built with [cursiv](https://github.com/ydaveluy/cursiv).

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
| The grammar | `src/bdf/core/BdfGrammar.hpp` |
| The AST | `src/bdf/core/ast.hpp` |
| Validation checks | `src/bdf/core/validation/` |
| Services of your own | `src/bdf/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/bdf/cli/main.cpp` |
| LSP features | `src/bdf/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
STARTFONT 2.1
FONT -gnu-unifont-medium-r-normal--16-160-75-75-c-80-iso10646-1
SIZE 16 75 75
FONTBOUNDINGBOX 16 16 0 -2
STARTPROPERTIES 2
FONT_ASCENT 14
FONT_DESCENT 2
ENDPROPERTIES
CHARS 1
STARTCHAR U+0041
ENCODING 65
SWIDTH 500 0
DWIDTH 8 0
BBX 8 16 0 -2
BITMAP
00
...
ENDCHAR
ENDFONT
```

A font is a flat sequence of keyword-led declarations: metrics
(`SIZE`, `FONTBOUNDINGBOX`), a block of named properties, and one block per
character, its bitmap a run of hex rows.

## The command line

```bash
./build/src/bdf_cli example/example1.bdf
./build/src/bdf_cli --dump example/example1.bdf   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`bdf_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
