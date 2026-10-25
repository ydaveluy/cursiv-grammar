# vba

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
| The grammar | `src/vba/core/VbaGrammar.hpp` |
| The AST | `src/vba/core/ast.hpp` |
| Validation checks | `src/vba/core/validation/` |
| Services of your own | `src/vba/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/vba/cli/main.cpp` |
| LSP features | `src/vba/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

VBA: Visual Basic for Applications, as written in `.bas` standard modules,
`.cls` class modules and `.frm` forms.

```vba
Attribute VB_Name = "Greeter"
Option Explicit

Public Function Greet(ByVal name As String) As String
    If Len(name) = 0 Then
        Greet = "Hello, stranger!"
    Else
        Greet = "Hello, " & name & "!"
    End If
End Function
```

A `.cls`/`.frm` file starts with `VERSION x [CLASS]` and a `BEGIN...END`
configuration block (a form's or class's designer metadata, not code) before
the same declarations-then-procedures body.

## The command line

```bash
./build/src/vba_cli example/SQLHelperFunctions.bas
./build/src/vba_cli --dump example/SQLHelperFunctions.bas   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`vba_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
