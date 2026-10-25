# morsecode

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
| The grammar | `src/morsecode/core/MorsecodeGrammar.hpp` |
| The AST | `src/morsecode/core/ast.hpp` |
| Validation checks | `src/morsecode/core/validation/` |
| Services of your own | `src/morsecode/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/morsecode/cli/main.cpp` |
| LSP features | `src/morsecode/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
... --- ...
```

A message is one or more letters separated by a single space; each letter is
a fixed run of `.` and `-`. `SOS` decodes to the letters `s`, `o`, `s`.

## The command line

```bash
./build/src/morsecode_cli example/SOS.morsecode
./build/src/morsecode_cli --dump example/SOS.morsecode   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`morsecode_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
