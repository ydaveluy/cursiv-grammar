# scotty

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
| The grammar | `src/scotty/core/ScottyGrammar.hpp` |
| The AST | `src/scotty/core/ast.hpp` |
| Validation checks | `src/scotty/core/validation/` |
| Services of your own | `src/scotty/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/scotty/cli/main.cpp` |
| LSP features | `src/scotty/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

Scotty ([esolangs.org/wiki/Scotty](https://esolangs.org/wiki/Scotty)) is
written fully prefixed: an operator names both its operands, so `+ 1 1` is
`1 + 1` and no operator ever needs a precedence table.

```
fun inc n = + n 1
fun double x = * x 2
five = 5
result = (double (inc five))
(inc -5)
```

A program is zero or more function definitions and variable assignments,
followed by one final expression — its result. `(f x)` calls a function on
one argument; a bare name is a cross reference to a variable or to the
enclosing function's parameter, resolved by the linker.

## The command line

```bash
./build/src/scotty_cli example/example1.scotty
./build/src/scotty_cli --dump example/example1.scotty   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`scotty_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
