# srt

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
| The grammar | `src/srt/core/SrtGrammar.hpp` |
| The AST | `src/srt/core/ast.hpp` |
| Validation checks | `src/srt/core/validation/` |
| Services of your own | `src/srt/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/srt/cli/main.cpp` |
| LSP features | `src/srt/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
1
00:02:07,000 --> 00:02:08,100
Ik ben er.

2
00:02:12,200 --> 00:02:14,100
Ja, ik ben er.
```

A [SubRip](https://en.wikipedia.org/wiki/SubRip) file is a sequence of
numbered blocks: an index, a `start --> end` timestamp range
(`hh:mm:ss,mmm`), and one or more lines of caption text, separated by a
blank line.

## The command line

```bash
./build/src/srt_cli example/srt_example.srt
./build/src/srt_cli --dump example/srt_example.srt   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`srt_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
