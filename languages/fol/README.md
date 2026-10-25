# fol

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
| The grammar | `src/fol/core/FolGrammar.hpp` |
| The AST | `src/fol/core/ast.hpp` |
| Validation checks | `src/fol/core/validation/` |
| Services of your own | `src/fol/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/fol/cli/main.cpp` |
| LSP features | `src/fol/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
_isHuman(#Sokrates)
Forall(?x) _isHuman(?x)->_isMortal(?x)
_isMortal(#Sokrates)
```

One formula per line. `#Sokrates` is an individual constant, `?x` a
variable, `_isHuman`/`_isMortal` predicates applied in parentheses, `Forall`
a quantifier over a parenthesized variable, and `->` one of the four binary
connectives (`\/`, `^`, `->`, `<->`) chained left-associatively between
formulas. `!` negates, `.name(...)` applies a function constant, and `=`
compares two terms.

## The command line

```bash
./build/src/fol_cli example/example1.fol
./build/src/fol_cli --dump example/example1.fol   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`fol_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
