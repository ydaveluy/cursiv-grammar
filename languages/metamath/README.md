# metamath

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
| The grammar | `src/metamath/core/MetamathGrammar.hpp` |
| The AST | `src/metamath/core/ast.hpp` |
| Validation checks | `src/metamath/core/validation/` |
| Services of your own | `src/metamath/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/metamath/cli/main.cpp` |
| LSP features | `src/metamath/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
$c 0 + = -> ( ) term wff |- $.
$v t r s P Q $.

tt $f term t $.
tr $f term r $.

weq $a wff t = r $.
wim $a wff ( P -> Q ) $.

ax-1 $a |- ( P -> ( Q -> P ) ) $.
thm1 $p |- ( P -> ( Q -> P ) ) $= ax-1 $.
```

A database is a flat sequence of `$c`/`$v` declarations, hypotheses (`$f`,
`$e`), assertions (`$a`, `$p`), grouped by `${ $}` blocks that scope
variables and hypotheses, and nothing else — Metamath has no expression
syntax of its own: everything after a keyword up to the closing `$.` is a
bare sequence of symbols. A `$p` statement carries a proof, either a list of
labels (and `?` for an unknown step) or a compressed run of capital letters
between parentheses.

## The command line

```bash
./build/src/metamath_cli tests/demo.mm
./build/src/metamath_cli --dump tests/demo.mm   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`metamath_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
