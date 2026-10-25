# calculator

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
| The grammar | `src/calculator/core/CalculatorGrammar.hpp` |
| The AST | `src/calculator/core/ast.hpp` |
| Validation checks | `src/calculator/core/validation/` |
| Services of your own | `src/calculator/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/calculator/cli/main.cpp` |
| LSP features | `src/calculator/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
A = pi*r^2
```

A calculator document is one equation: an expression, a relational operator
(`=`, `>`, `<`), and another expression. An expression is the usual
arithmetic ladder (`^` tightest, then `*`/`/`, then `+`/`-`, with left
associativity and a `+`/`-` prefix chain on atoms), over numbers, variables,
the constants `pi`, `e`, `i`, parenthesized groups, and calls to `cos`,
`sin`, `tan`, `acos`, `asin`, `atan`, `log`, `ln` and `sqrt` — with any
number of comma-separated arguments, since the grammar does not fix an
arity.

## The command line

```bash
./build/src/calculator_cli example/area.calc
./build/src/calculator_cli --dump example/area.calc   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`calculator_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
