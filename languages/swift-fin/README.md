# swiftfin

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
| The grammar | `src/swiftfin/core/SwiftFinGrammar.hpp` |
| The AST | `src/swiftfin/core/ast.hpp` |
| Validation checks | `src/swiftfin/core/validation/` |
| Services of your own | `src/swiftfin/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/swiftfin/cli/main.cpp` |
| LSP features | `src/swiftfin/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

A S.W.I.F.T. FIN message: five numbered blocks, braces delimited, each
optional but block 1. Block 4, the text block, holds the actual message
fields (`:20:reference`, `:23G:instruction`, ...), one per line, continuation
lines allowed; blocks 3 and 5 (and block 4 itself, in its other form) are
flat `{key:value}` maps.

```
{1:F01ASDFQQQQAXXX0103027010}{2:O9502015150713ASDFQQQQAXXX91266335941507132015N}{3:{108:55094429070-07-G}}{4:
:20:55094429070-0713
:25:999-55.094.429
:28C:00135/00001
:60F:C150710USD2470123,19
-}
```

## The command line

```bash
./build/src/swiftfin_cli example/complex2.swiftfin
./build/src/swiftfin_cli --dump example/complex2.swiftfin   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`swiftfin_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
