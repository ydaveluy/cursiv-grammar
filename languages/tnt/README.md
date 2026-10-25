# tnt

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
| The grammar | `src/tnt/core/TntGrammar.hpp` |
| The AST | `src/tnt/core/ast.hpp` |
| Validation checks | `src/tnt/core/validation/` |
| Services of your own | `src/tnt/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/tnt/cli/main.cpp` |
| LSP features | `src/tnt/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
Aa:Ab:(a + b) = (b + a)
```

A [TNT](http://en.wikipedia.org/wiki/Typographical_Number_Theory) formula:
`0` is zero, `S` a successor (`SS0` is two), a lowercase letter `a`-`e` a
variable (`a''` a distinct one, `Sb` its successor), `A`/`E` a universal or
existential quantifier over a variable, `~` negation, and `+`/`*` the two
operations — all inside one equation, `expression '=' expression`.

## The command line

```bash
./build/src/tnt_cli example/commutative.tnt
./build/src/tnt_cli --dump example/commutative.tnt   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`tnt_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
