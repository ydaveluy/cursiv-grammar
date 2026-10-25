# modula2pim4

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
| The grammar | `src/modula2pim4/core/Modula2pim4Grammar.hpp` |
| The AST | `src/modula2pim4/core/ast.hpp` |
| Validation checks | `src/modula2pim4/core/validation/` |
| Services of your own | `src/modula2pim4/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/modula2pim4/cli/main.cpp` |
| LSP features | `src/modula2pim4/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
MODULE Main;

IMPORT IO;       (* So we can print things *)

VAR name: TEXT;  (* a string variable called "name" *)

BEGIN
  IO.Put("Enter your name: ");
  name:=IO.GetLine();
  IO.Put("Your name is: " & name & "\n");
END Main.
```

Modula-2 (PIM, 4th edition): Wirth's language between Pascal and Oberon. A
compilation unit is a `MODULE` (a program, optionally prefixed
`IMPLEMENTATION`) or a `DEFINITION MODULE` (an interface); modules can
import from and nest inside one another. This port is syntax-only: names are
plain text, never resolved.

## The command line

```bash
./build/src/modula2pim4_cli example/example1.mod
./build/src/modula2pim4_cli --dump example/example1.mod   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`modula2pim4_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
