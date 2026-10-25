# lcc

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
| The grammar | `src/lcc/core/LccGrammar.hpp` |
| The AST | `src/lcc/core/ast.hpp` |
| Validation checks | `src/lcc/core/validation/` |
| Services of your own | `src/lcc/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/lcc/cli/main.cpp` |
| LSP features | `src/lcc/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
QB981 .H377 1998
```

A [Library of Congress Classification](https://en.wikipedia.org/wiki/Library_of_Congress_Classification)
call number: topic `Q` (Science), subtopic `B` (Astronomy), subclass `981`,
cutter `.H377`, date `1998`. Space is significant and never trivia -- only
tab, CR and LF are skipped, so it appears in the grammar wherever the number
allows or requires it.

## The command line

```bash
./build/src/lcc_cli example/eg1.lcc
./build/src/lcc_cli --dump example/eg1.lcc   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`lcc_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
