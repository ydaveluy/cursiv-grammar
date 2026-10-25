# quakemap

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
| The grammar | `src/quakemap/core/QuakemapGrammar.hpp` |
| The AST | `src/quakemap/core/ast.hpp` |
| Validation checks | `src/quakemap/core/validation/` |
| Services of your own | `src/quakemap/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/quakemap/cli/main.cpp` |
| LSP features | `src/quakemap/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
{
"classname" "info_player_start"
"origin" "32 32 24"
{
( 256 64 16 ) ( 256 64 0 ) ( 256 0 16 ) mmetal1_2 0 0 0 1 1
( 0 0 0 ) ( 0 64 0 ) ( 0 0 16 ) mmetal1_2 0 0 0 1 1
}
}
```

A [Quake map](https://quakewiki.org/wiki/Quake_Map_Format) is a list of
entities. Each entity is a `"key" "value"` string pair list (its properties)
followed by zero or more brushes — the convex solids that carve its geometry.
A brush is one or more planes: three points, a texture name, then the
texture's alignment numbers.

## The command line

```bash
./build/src/quakemap_cli example/example1.map
./build/src/quakemap_cli --dump example/example1.map   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`quakemap_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
