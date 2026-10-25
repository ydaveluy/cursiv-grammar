# geekcode

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
| The grammar | `src/geekcode/core/GeekcodeGrammar.hpp` |
| The AST | `src/geekcode/core/ast.hpp` |
| Validation checks | `src/geekcode/core/validation/` |
| Services of your own | `src/geekcode/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/geekcode/cli/main.cpp` |
| LSP features | `src/geekcode/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
GED/J d-- s:++>: a-- C++(++++) ULU++ P+ L++ E---- W+(-) N+++ o+ K+++
w--- O- M+ V-- PS++>$ PE++>$ Y++ PGP++ t- 5+++ X++ R+++>$ tv+ b+ DI+++
D+++ G+++++ e++ h r-- y++**
```

A geek code: a header naming the author's interest groups (`GED/J`), then one
tuple per category. A tuple is a code (`d` dress, `C` computers, a two-letter
`PS`/`PE`/`PGP`...), optionally prefixed with `!` (refuses the whole
category) and followed by any mix of sizing runs (`-`/`+`/`*`), a
parenthesized sizing (`(++++)`), and qualifier marks (`@`/`>`/`$`/`?`). Two
codes are structured rather than flat: `s` opens a shape (segments separated
by `:`, each with its own sizing and qualifiers), and `U` opens a run of UNIX
flavors (`UB` BSD, `UL` Linux via the shared `L` code, a bare `U` repeated for
"Unix in general").

## The command line

```bash
./build/src/geekcode_cli example/example1.geek
./build/src/geekcode_cli --dump example/example1.geek   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`geekcode_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
