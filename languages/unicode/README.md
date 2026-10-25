# graphemes

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
| The grammar | `src/graphemes/core/GraphemesGrammar.hpp` |
| The AST | `src/graphemes/core/ast.hpp` |
| Validation checks | `src/graphemes/core/validation/` |
| Services of your own | `src/graphemes/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/graphemes/cli/main.cpp` |
| LSP features | `src/graphemes/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

Grapheme-cluster breaking (UAX #29 / UTS #51): a `.graphemes` document is
raw Unicode text, cut into the "user-perceived characters" a text editor's
arrow keys would step over one at a time.

```
👮‍♀️🖖🏽🇨🇨
```

is three clusters — a ZWJ sequence (base + gender), a skin-toned wave, a
flag — never the nine or so code points a codepoint-by-codepoint reading
would see.

## The command line

```bash
./build/src/graphemes_cli example/ascii.graphemes
./build/src/graphemes_cli --dump example/emoji.graphemes   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`graphemes_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
