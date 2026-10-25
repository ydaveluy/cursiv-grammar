# odin

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
| The grammar | `src/odin/core/OdinGrammar.hpp` |
| The AST | `src/odin/core/ast.hpp` |
| Validation checks | `src/odin/core/validation/` |
| Services of your own | `src/odin/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/odin/cli/main.cpp` |
| LSP features | `src/odin/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```odin
package main

import "core:fmt"

main :: proc() {
    fmt.println("Hellope!")
    i, j : int = 1, 2
}
```

A package of declarations: constants, variables, procedures, types and
imports, each written with `::` (constant), `:=` (inferred variable) or `:`
(typed variable). `example/` carries the language's own upstream corpus —
a short `test.odin` and a long, comprehensive `demo.odin` tour.

## The command line

```bash
./build/src/odin_cli example/test.odin
./build/src/odin_cli --dump example/test.odin   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`odin_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
