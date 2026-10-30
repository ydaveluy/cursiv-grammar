# teal

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
| The grammar | `src/teal/core/TealGrammar.hpp` |
| The AST | `src/teal/core/ast.hpp` |
| Validation checks | `src/teal/core/validation/` |
| Services of your own | `src/teal/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/teal/cli/main.cpp` |
| LSP features | `src/teal/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

A partial Teal 0.24.8 syntax parser with typed Lua statements and named type declarations. It builds syntax trees without Teal type checking.

```lua
local record Point
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
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
