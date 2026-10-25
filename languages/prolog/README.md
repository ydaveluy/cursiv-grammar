# prolog

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
| The grammar | `src/prolog/core/PrologGrammar.hpp` |
| The AST | `src/prolog/core/ast.hpp` |
| Validation checks | `src/prolog/core/validation/` |
| Services of your own | `src/prolog/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/prolog/cli/main.cpp` |
| LSP features | `src/prolog/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```prolog
has(jack,apples).
has(ann,plums).
has(dan,money).
fruit(apples).
fruit(plums).
```

Facts: each is a `clause`, a `term` (here a `compound_term`, a functor
applied to arguments) closed by `.`. `example/constructs.pl` exercises the
rest of the language — directives, operators, lists, variables, the three
quoted forms, nested comments, DCG rules.

## The command line

```bash
./build/src/prolog_cli example/example1.pl
./build/src/prolog_cli --dump example/example1.pl   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`prolog_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
