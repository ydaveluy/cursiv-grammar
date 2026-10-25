# ada

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
| The grammar | `src/ada/core/AdaGrammar.hpp` |
| The AST | `src/ada/core/ast.hpp` |
| Validation checks | `src/ada/core/validation/` |
| Services of your own | `src/ada/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/ada/cli/main.cpp` |
| LSP features | `src/ada/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```ada
with Ada.Text_IO; use Ada.Text_IO;

procedure Hello is
begin
   Put_Line ("Hello, world!");
end Hello;
```

Names are kept as written, not resolved: a document routinely mentions units
and predefined entities outside itself (`Ada.Text_IO` above), so there is no
cross-reference linker here.

## The command line

```bash
./build/src/ada_cli example/pkg1.ada
./build/src/ada_cli --dump example/pkg1.ada   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`ada_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
