# aterm

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
| The grammar | `src/aterm/core/AtermGrammar.hpp` |
| The AST | `src/aterm/core/ast.hpp` |
| Validation checks | `src/aterm/core/validation/` |
| Services of your own | `src/aterm/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/aterm/cli/main.cpp` |
| LSP features | `src/aterm/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
exam(<appl(<term>,9)>,<int>,<str>)
```

An [ATerm](https://homepages.cwi.nl/~daybuild/daily-books/technology/aterm-guide/aterm-guide.html)
document is one term: a function application (`f(a,b)`, or just `f` for zero
arguments), a number, a list (`[a,b]`), or a placeholder (`<a>`) marking a
hole to fill in later. Any term may carry a trailing `{...}` annotation.

## The command line

```bash
./build/src/aterm_cli example/example2.aterm
./build/src/aterm_cli --dump example/example2.aterm   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`aterm_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
