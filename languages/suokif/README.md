# suokif

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
| The grammar | `src/suokif/core/SuokifGrammar.hpp` |
| The AST | `src/suokif/core/ast.hpp` |
| Validation checks | `src/suokif/core/validation/` |
| Services of your own | `src/suokif/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/suokif/cli/main.cpp` |
| LSP features | `src/suokif/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

SUO-KIF is the Lisp-like sentence syntax of SUMO (the Standard Upper Merged
Ontology): a document is a run of parenthesized sentences over words,
variables, strings and numbers.

```
(instance Cat Animal)
(=> (instance ?X Dog) (instance ?X Animal))
(exists (?X ?Y) (and (instance ?X Dog) (instance ?Y Cat) (not (equal ?X ?Y))))
```

A relation's head is a word or a variable, its arguments any term —
including a nested sentence, since SUO-KIF sentences are themselves terms.

## The command line

```bash
./build/src/suokif_cli example/example1.kif
./build/src/suokif_cli --dump example/example1.kif   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`suokif_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
