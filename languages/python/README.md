# python

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
| The grammar | `src/python/core/PythonGrammar.hpp` |
| The AST | `src/python/core/ast.hpp` |
| Validation checks | `src/python/core/validation/` |
| Services of your own | `src/python/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/python/cli/main.cpp` |
| LSP features | `src/python/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```python
def describe(point):
    match point:
        case Point(x=0, y=0):
            return "origin"
        case Point(x=0, y=y) | Point(x=y, y=0):
            return f"on an axis at {y}"
        case _:
            return "elsewhere"
```

Python 3.14's full statement and expression grammar: `def`/`class`,
`if`/`while`/`for`/`with`/`try`, `match`/`case` with the full pattern grammar,
comprehensions, f-strings and t-strings, PEP 695 type parameters. Suites are
indentation-delimited (`indent`/`dedent`) or, where the reference grammar
itself allows it, written on the same line as their header (`if x: y = 1`).

## The command line

```bash
./build/src/python_cli example/__hello__.py
./build/src/python_cli --dump example/__hello__.py   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`python_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
