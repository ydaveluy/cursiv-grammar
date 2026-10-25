# rpn

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
| The grammar | `src/rpn/core/RpnGrammar.hpp` |
| The AST | `src/rpn/core/ast.hpp` |
| Validation checks | `src/rpn/core/validation/` |
| Services of your own | `src/rpn/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/rpn/cli/main.cpp` |
| LSP features | `src/rpn/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
2 2 * 3 3 * +
```

A [reverse Polish notation](https://en.wikipedia.org/wiki/Reverse_Polish_notation)
expression: an operand, then a run of operands and operators applied left to
right against a stack — here, `2*2 + 3*3`. An operand is a number (plain,
decimal, or with an `e`/`E` exponent) or a variable name, optionally signed
(`-x`, `- -5`); an operator is `^ + - * /` or one of the unary functions
`cos sin tan acos asin atan log ln`.

## The command line

```bash
./build/src/rpn_cli example/pythagoras.rpn
./build/src/rpn_cli --dump example/pythagoras.rpn   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`rpn_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
