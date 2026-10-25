# pest

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
| The grammar | `src/pest/core/PestGrammar.hpp` |
| The AST | `src/pest/core/ast.hpp` |
| Validation checks | `src/pest/core/validation/` |
| Services of your own | `src/pest/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/pest/cli/main.cpp` |
| LSP features | `src/pest/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

A pest document is itself a grammar description — the syntax [pest](https://pest.rs)
uses to describe the language it parses, PEG rules over rules:

```
digit = { '0'..'9' }
num   = @{ digit+ ~ ("." ~ digit*)? }
expr  = { num ~ ((add | subtract) ~ num)* }
add      = { "+" }
subtract = { "-" }

WHITESPACE = _{ " " | "\t" }
```

`=` declares a rule; `_`/`@`/`$`/`!` before the `{` mark it silent, atomic,
compound-atomic or non-atomic; `~` sequences, `|` chooses; `'a'..'z'` is a
character range, and `PUSH(...)` / `PEEK[..]` read and match the rule's own
stack. `example/` contains sample grammars, including `features.pest` for
additional constructs.

## The command line

```bash
./build/src/pest_cli example/json.pest
./build/src/pest_cli --dump example/json.pest   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`pest_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
