# teal

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
| The grammar | `src/teal/core/TealGrammar.hpp` |
| The AST | `src/teal/core/ast.hpp` |
| Validation checks | `src/teal/core/validation/` |
| Services of your own | `src/teal/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/teal/cli/main.cpp` |
| LSP features | `src/teal/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```lua
local Point = record
   x: number
   y: number
end

local function distance(a: Point, b: Point): number
   local dx: number = a.x - b.x
   local dy: number = b.y - a.y
   return (dx * dx + dy * dy) ^ 0.5
end

local origin: Point = { x = 0, y = 0 }
```

Teal is Lua's syntax with a type annotation layer over it: a `record`
declares a named type, `NAME: typ` annotates a local, parameter or field, and
ordinary Lua statements and expressions carry on unchanged underneath.

## The command line

```bash
./build/src/teal_cli example/functions.tl
./build/src/teal_cli --dump example/functions.tl   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`teal_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
