# asl

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
| The grammar | `src/asl/core/AslGrammar.hpp` |
| The AST | `src/asl/core/ast.hpp` |
| Validation checks | `src/asl/core/validation/` |
| Services of your own | `src/asl/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/asl/cli/main.cpp` |
| LSP features | `src/asl/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

ASL (the Action Specification Language) is an action language for
Shlaer-Mellor / UML state models: assignments, object creation and finds,
relationship navigation, event generation and control flow, one statement
per line.

```
detector = find-one METS_DETECTOR where Number = 1
if detector != UNDEFINED then
  %generate DEVCDCTR2:Valid_Device(detector.Address); to this
else
  %generate DEVCDCTR3:Invalid_Device(); to this
endif
```

A statement is one line (a backslash continues it); `{name}` is a set or a
struct instance; `R34` is a relationship number, `R34.OBJECT` or
`R34."role phrase"` a qualified traversal of it. See `example/` for every
construct: creation, deletion, finds, links, timers, sets, structs, loops.

## The command line

```bash
./build/src/asl_cli example/assignments.asl
./build/src/asl_cli --dump example/assignments.asl   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`asl_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
