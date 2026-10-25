# arden

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
| The grammar | `src/arden/core/ArdenGrammar.hpp` |
| The AST | `src/arden/core/ast.hpp` |
| Validation checks | `src/arden/core/validation/` |
| Services of your own | `src/arden/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/arden/cli/main.cpp` |
| LSP features | `src/arden/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

An Arden Syntax Medical Logic Module (`.mlm`): four categories --
`maintenance`, `library`, `knowledge`, `resources` -- each a run of
`slot: value;;` pairs, closed by `end:`.

```
maintenance:
title: Check for penicillin allergy;;
mlmname: pen_allergy;;
arden: version 3;;
version: 1.00;;
institution: Columbia-Presbyterian Medical Center;;
author: George Hripcsak, M.D.;;
specialist: ;;
date: 1991-03-18;;
validation: testing;;
library:
purpose: When a penicillin is prescribed, check for an allergy.;;
explanation: Evoked when a penicillin medication is ordered.;;
keywords: penicillin; allergy;;
citations: ;;
knowledge:
type: data-driven;;
data: penicillin_order := event {medication_order where class = penicillin};
penicillin_allergy := read last {allergy where agent_class = penicillin};
;;
evoke: penicillin_order;
;;
logic: if exist(penicillin_allergy) then
conclude true;
endif;
;;
action: write "Caution: an allergy to penicillin is documented." || penicillin_allergy;
;;
urgency: 50;;
resources:
default: en;;
language: en;;
end:
```

The four categories are named nodes (`ast::Mlm::maintenance` and friends);
`data`/`logic`/`action` hold statement trees built from the same shared
assignment, `if`/`for`/`while`/`switch` shapes.

## The command line

```bash
./build/src/arden_cli "example/Contraindication Alert.mlm"
./build/src/arden_cli --dump "example/Contraindication Alert.mlm"   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`arden_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
