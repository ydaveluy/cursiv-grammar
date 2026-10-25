# ocl

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
| The grammar | `src/ocl/core/OclGrammar.hpp` |
| The AST | `src/ocl/core/ast.hpp` |
| Validation checks | `src/ocl/core/validation/` |
| Services of your own | `src/ocl/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/ocl/cli/main.cpp` |
| LSP features | `src/ocl/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
context Person inv:
    statistics->sortedBy(totalSalary)->last().wellpaidEmployees->includes(self)

context MyClass::derivedAttribute: Integer
    derive: attribute1 + attribute2
```

A document is one or more OCL context specifications: an invariant
(`context Type inv name?: expression`) or a derived-attribute definition
(`context Type::attr: type (init: expression)? derive: expression`). The
expression language is OCL 2.4 -- infix operators, `if`/`then`/`else`,
`let`/`in`, collection literals, and the `->`/`.` operation chains
(`select`, `collect`, `forAll`, `iterate`...) that give OCL its shape.

## The command line

```bash
./build/src/ocl_cli example/contextExamples.ocl
./build/src/ocl_cli --dump example/contextExamples.ocl   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`ocl_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
