# swiftfin

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
| The grammar | `src/swiftfin/core/SwiftFinGrammar.hpp` |
| The AST | `src/swiftfin/core/ast.hpp` |
| Validation checks | `src/swiftfin/core/validation/` |
| Services of your own | `src/swiftfin/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/swiftfin/cli/main.cpp` |
| LSP features | `src/swiftfin/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

A structural S.W.I.F.T. FIN message parser: five numbered blocks, braces delimited, each
optional but block 1. Block 4, the text block, holds the actual message
fields (`:20:reference`, `:23G:instruction`, ...), one per line, continuation
lines allowed; blocks 3 and 5 (and block 4 itself, in its other form) are
`{key:value}` maps whose values retain nested source text.

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
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
