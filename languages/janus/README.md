# janus

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
| The grammar | `src/janus/core/JanusGrammar.hpp` |
| The AST | `src/janus/core/ast.hpp` |
| Validation checks | `src/janus/core/validation/` |
| Services of your own | `src/janus/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/janus/cli/main.cpp` |
| LSP features | `src/janus/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

[Janus](https://en.wikipedia.org/wiki/Janus_(time-reversible_computing_programming_language))
is Christopher Bennett's reversible imperative language: every statement has
an inverse, so a program runs backwards as easily as forwards.

```
n acc history[9]

PROCEDURE double
  READ n
  n += n
  WRITE n

PROCEDURE main
  CALL double
  UNCALL double
  ...
```

`CALL`/`UNCALL` run a procedure forwards or in reverse; `+=`/`-=` and `<=>`
mutate in place (their own inverses); `:` swaps two variables; `FROM ...
DO ... LOOP ... UNTIL` and `IF ... THEN ... ELSE ... FI` carry the assertions
reversibility needs at both ends.

## The command line

```bash
./build/src/janus_cli example/example1.janus
./build/src/janus_cli --dump example/example1.janus   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`janus_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
