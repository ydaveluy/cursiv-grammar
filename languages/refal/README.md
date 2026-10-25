# refal

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
| The grammar | `src/refal/core/RefalGrammar.hpp` |
| The AST | `src/refal/core/ast.hpp` |
| Validation checks | `src/refal/core/validation/` |
| Services of your own | `src/refal/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/refal/cli/main.cpp` |
| LSP features | `src/refal/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
$ENTRY Go { = <Prout <Pal 'revolver'>> }

Pal { = True;
     s.1 = True;
     s.1 e.2 s.1 = <Pal e.2>;
     e.1 = False;  }
```

A Refal program is definitions: functions (`$ENTRY` marks the one execution
starts at) and `$EXTERNAL`/`$EXTERN`/`$EXTRN` declarations. A function is
sentences, each a pattern matched left to right against the call, guard
conditions (`, arg : pattern`), and either a result expression or a nested
block (`, arg : { ... }`). A pattern or a result is symbols (words, numbers,
quoted text, escapes), pattern variables (`s.name` one symbol, `t.name` one
term, `e.name` any run), and nested calls (`<...>`), evaluated inside out.

## The command line

```bash
./build/src/refal_cli example/example1.ref
./build/src/refal_cli --dump example/example1.ref   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`refal_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
