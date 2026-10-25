# oncrpc

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
| The grammar | `src/oncrpc/core/OncrpcGrammar.hpp` |
| The AST | `src/oncrpc/core/ast.hpp` |
| Validation checks | `src/oncrpc/core/validation/` |
| Services of your own | `src/oncrpc/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/oncrpc/cli/main.cpp` |
| LSP features | `src/oncrpc/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

XDR (RFC 4506) type and constant definitions, and ONCRPCv2's (RFC 5531)
`program`/`version`/procedure declarations on top:

```
struct CalculationResult {
    hyper  result;
    unsigned hyper startMillis;
    unsigned hyper finishMillis;
};

program CALCULATOR {
    version CALCULATORVERS {
        CalculationResult add(hyper, hyper) = 1;
        hyper addSimple(hyper, hyper) = 2;
    } = 1;
} = 117;
```

`example/types.x` exercises the rest: `const`, `typedef`, fixed and variable
arrays, `opaque`, `string`, pointers, a discriminated `union` with a
`default` arm, and anonymous `enum`/`struct` type specifiers.

## The command line

```bash
./build/src/oncrpc_cli example/CalculatorService.x
./build/src/oncrpc_cli --dump example/CalculatorService.x   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`oncrpc_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
