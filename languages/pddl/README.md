# pddl

A language library built with [cursiv](https://github.com/ydaveluy/cursiv).

## Build

Requires CMake 3.30+, Git and a C++20 compiler with `std::format` and `musttail`
support. Cursiv's CI uses GCC 16 and Clang 22.

```bash
cmake -B build
cmake --build build -j
ctest --test-dir build --output-on-failure
```

The build generates the parser automatically. Rebuild after changing the grammar.

## What to edit

| What you want to change | File |
|---|---|
| The grammar | `src/pddl/core/PddlGrammar.hpp` |
| The AST | `src/pddl/core/ast.hpp` |
| Validation checks | `src/pddl/core/validation/` |
| Services of your own | `src/pddl/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/pddl/cli/main.cpp` |
| LSP features | `src/pddl/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
(define (domain blocksworld)
  (:requirements :strips :typing)
  (:types block table - object)
  (:predicates (on ?x - block ?y - object) (clear ?x - object))
  (:action put-down
   :parameters (?x - block ?y - object)
   :precondition (clear ?y)
   :effect (and (on ?x ?y) (not (clear ?y)))))
```

PDDL (the Planning Domain Definition Language) has no upstream example
corpus, so `example/` is empty; the documents this port exercises live under
`tests/` -- a blocksworld domain/problem pair, and a durative-action
rover-transport pair that also carries derived predicates, duration and
temporal constraints, and a metric.

## The command line

```bash
./build/src/pddl_cli tests/blocksworld-domain.pddl
./build/src/pddl_cli --dump tests/blocksworld-domain.pddl   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`pddl_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- GCC/Clang presets run the host binary, WASI presets the wasm module -- and a
second window opens with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
