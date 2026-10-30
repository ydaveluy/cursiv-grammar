# trapc

A subset of the 2025 TrapC proposal, centered on C syntax and `trap` blocks,
built with [cursiv](https://github.com/ydaveluy/cursiv).

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
| The grammar | `src/trapc/core/TrapcGrammar.hpp` |
| The AST | `src/trapc/core/ast.hpp` |
| Validation checks | `src/trapc/core/validation/` |
| Services of your own | `src/trapc/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/trapc/cli/main.cpp` |
| LSP features | `src/trapc/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```c
int main()
{   int x = 0;
    x = 1/0; trap
    {
        return 1;
    }
    return 0;
}
```

This syntax port covers C declarations, expressions and control flow with
`trap` blocks; see `example/hello.tc` for ordinary C syntax.

## The command line

```bash
./build/src/trapc_cli example/trap_test.tc
./build/src/trapc_cli --dump example/trap_test.tc   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`trapc_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
