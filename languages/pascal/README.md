# pascal

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
| The grammar | `src/pascal/core/PascalGrammar.hpp` |
| The AST | `src/pascal/core/ast.hpp` |
| Validation checks | `src/pascal/core/validation/` |
| Services of your own | `src/pascal/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/pascal/cli/main.cpp` |
| LSP features | `src/pascal/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```pascal
program HelloWorld;
begin
 write('Hello World.');
 readln;
end.
```

Procedural Pascal: programs and units, declarations
(`const`/`type`/`var`/`label`/`uses`), procedures and functions (nested, `var`
parameters), the structured statements, and selected types, including variant records and procedural types. `example/` contains programs such as
`helloworld.pas`, `schedule.pas`, `variants.pas` and `altsyntax.pas`.

## The command line

```bash
./build/src/pascal_cli example/helloworld.pas
./build/src/pascal_cli --dump example/helloworld.pas   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`pascal_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
