# guido

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
| The grammar | `src/guido/core/GuidoGrammar.hpp` |
| The AST | `src/guido/core/ast.hpp` |
| Validation checks | `src/guido/core/validation/` |
| Services of your own | `src/guido/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/guido/cli/main.cpp` |
| LSP features | `src/guido/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

[GUIDO Music Notation](https://en.wikipedia.org/wiki/GUIDO_music_notation) is
a text format for a musical score: a sequence of notes and tags between
brackets.

```
[ \clef<"treble"> \key<"D"> \meter<"4/4">
  a1*1/2 b a/4. g/8 f#/4 g a/2 b a/4. g/8 f#/4 g
  a/2 a b c#2/4 d c#/2 b1 a/1 ]
```

A note is a letter (`a`-`g`, or `_` for a rest), an optional accidental (`#`
or `&`), an optional octave, an optional duration (`*`, a numerator, `/`, a
denominator) and optional dots. A tag (`\clef<"treble">`) names a property
and takes parameters between `<` `>`; some carry a parenthesised note list of
their own (`\t<"Cm7", 13> ({e&1*1, g1})`), a chord annotated with a chord
symbol. Braces group several bracketed sequences into a segment — a staff.

## The command line

```bash
./build/src/guido_cli example/example1.gmn
./build/src/guido_cli --dump example/example1.gmn   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`guido_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
