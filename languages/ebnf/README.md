# ebnf

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
| The grammar | `src/ebnf/core/EbnfGrammar.hpp` |
| The AST | `src/ebnf/core/ast.hpp` |
| Validation checks | `src/ebnf/core/validation/` |
| Services of your own | `src/ebnf/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/ebnf/cli/main.cpp` |
| LSP features | `src/ebnf/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
<greeting> ::= <salutation> <name>
<salutation> ::= hello | hi | good morning | good evening
<name> ::= Alice | Bob | Carol | well-known guest
```

BNF: a document is a list of `<name> ::= alternatives` rules; an alternative
is a run of words and `<name>` references, `|`-separated, with `[optional]`,
`{zero-or-more}` and `(one-or-more)` grouping.

## The command line

```bash
./build/src/ebnf_cli example/postal.ebnf
./build/src/ebnf_cli --dump example/postal.ebnf   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`ebnf_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
