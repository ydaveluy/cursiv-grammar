# trapc

TrapC, a memory-safe C dialect, built with
[cursiv](https://github.com/ydaveluy/cursiv).

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

TrapC is C (declarations, structs, enums, pointers, control flow, the full
expression grammar) with one addition and two removals: `trap { ... }` runs a
block whose faults are meant to be caught, and `union`/`goto` are gone. Plain
C source still parses: see `example/hello.tc`.

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
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
