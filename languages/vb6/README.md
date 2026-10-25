# vb6

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
| The grammar | `src/vb6/core/Vb6Grammar.hpp` |
| The AST | `src/vb6/core/ast.hpp` |
| Validation checks | `src/vb6/core/validation/` |
| Services of your own | `src/vb6/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/vb6/cli/main.cpp` |
| LSP features | `src/vb6/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```vb
Private Sub cmdHello_Click()
    txtHello.Text = "Hello World!"
    With txtHello
        .Font = "Arial"
        .FontSize = 16
    End With
End Sub
```

A Visual Basic 6 module: a form's, a class's or a plain code module's source
-- declarations, procedures and statements in one file, line-oriented (`:`
chains several statements on one line, `_` continues one across a line
break).

## The command line

```bash
./build/src/vb6_cli example/calls/Calls.cls
./build/src/vb6_cli --dump example/calls/Calls.cls   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`vb6_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
