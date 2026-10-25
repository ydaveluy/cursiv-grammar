# ron

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
| The grammar | `src/ron/core/RonGrammar.hpp` |
| The AST | `src/ron/core/ast.hpp` |
| Validation checks | `src/ron/core/validation/` |
| Services of your own | `src/ron/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/ron/cli/main.cpp` |
| LSP features | `src/ron/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

[RON](https://github.com/ron-rs/ron) (Rusty Object Notation) is a data format
close to JSON, with an optional leading type name on any value, single- and
double-quoted scalars, and trailing commas everywhere a list can end:

```
GameConfig( // optional struct name
    window_size: (800, 600),
    fullscreen: false,
    key_bindings: {
        "up": Up,
        "down": Down,
    },
)
```

`GameConfig` is a classname naming the struct that follows; `(800, 600)` is a
tuple; `Up` and `Down` are bare identifiers, RON's unit values (an enum
variant or a unit struct with no fields).

## The command line

```bash
./build/src/ron_cli example/example1.ron
./build/src/ron_cli --dump example/example1.ron   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`ron_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
