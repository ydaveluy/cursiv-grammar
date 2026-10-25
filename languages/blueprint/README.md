# blueprint

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
| The grammar | `src/blueprint/core/BlueprintGrammar.hpp` |
| The AST | `src/blueprint/core/ast.hpp` |
| Validation checks | `src/blueprint/core/validation/` |
| Services of your own | `src/blueprint/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/blueprint/cli/main.cpp` |
| LSP features | `src/blueprint/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

Blueprint (`Android.bp`) is the module-declaration language of the Android
build system: top-level variable assignments and module blocks, values that
can be a bool, an integer, a string, a list, a nested map, a variable
reference, or a `select()` on a product/arch condition.

```
cc_library {
    name: "example2",
    enable: select((arch(), os()), {
        (any @ bar, default): true,
    }),
    foobar: {
        value: foo + "bar",
    },
}
```

`example/example1.bp` is a sample document; `example/features.bp` demonstrates
additional constructs (`+=`, a module in `(...)` form, a negative integer, an
escaped quote, an unparenthesized `select` condition, a `default: unset`
case).

## The command line

```bash
./build/src/blueprint_cli example/example1.bp
./build/src/blueprint_cli --dump example/example1.bp   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`blueprint_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
