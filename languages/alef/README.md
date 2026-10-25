# alef

A language library built with [cursiv](https://github.com/ydaveluy/cursiv).

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
| The grammar | `src/alef/core/AlefGrammar.hpp` |
| The AST | `src/alef/core/ast.hpp` |
| Validation checks | `src/alef/core/validation/` |
| Services of your own | `src/alef/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/alef/cli/main.cpp` |
| LSP features | `src/alef/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
(int, byte*, byte)
func()
{
    return (10, "hello", 'c');
}

void
main()
{
    int a;
    byte* str;
    byte c;
    (a, str, c) = func();
}
```

Alef is Plan 9's concurrent C-family language: a function's type comes before
its name as in C, and a parenthesised type list like `(int, byte*, byte)`
declares a tuple, whose values a matching tuple pattern on the left of `=`
unpacks.

## The command line

```bash
./build/src/alef_cli example/example1.alef
./build/src/alef_cli --dump example/example1.alef   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`alef_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
