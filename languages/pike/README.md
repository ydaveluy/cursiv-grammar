# pike

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
| The grammar | `src/pike/core/PikeGrammar.hpp` |
| The AST | `src/pike/core/ast.hpp` |
| Validation checks | `src/pike/core/validation/` |
| Services of your own | `src/pike/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/pike/cli/main.cpp` |
| LSP features | `src/pike/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```pike
int main()
{
    write("hello world\n");
    return 0;
}
```

A Pike source file: type declarations, function definitions, and a body of
C-like statements and expressions. `example/` contains sample programs;
`tests/fixtures/kitchen_sink.pike` demonstrates imports, inheritance, classes,
`catch`/`gauge`/`sscanf`, lambdas, casts, and the array/mapping/multiset
literals.

## The command line

```bash
./build/src/pike_cli example/example1.pike
./build/src/pike_cli --dump example/example1.pike   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`pike_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
