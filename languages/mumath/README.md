# mumath

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
| The grammar | `src/mumath/core/MumathGrammar.hpp` |
| The AST | `src/mumath/core/ast.hpp` |
| Validation checks | `src/mumath/core/validation/` |
| Services of your own | `src/mumath/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/mumath/cli/main.cpp` |
| LSP features | `src/mumath/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
INT (LOG (X)^10,X);
```

A [MuMath](https://en.wikipedia.org/wiki/MuMATH) program is a sequence of
function definitions, assignments and calls, each ended by `;` or `$`.
Identifiers are uppercase words, optionally starting with `@`, `{` or `#`;
`FUNCTION`/`ENDFUN`, `LOOP`/`ENDLOOP`, `BLOCK`/`ENDBLOCK` and `WHEN`/`EXIT`
delimit function bodies and their nested control constructs.

## The command line

```bash
./build/src/mumath_cli example/example1.mu
./build/src/mumath_cli --dump example/example1.mu   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`mumath_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
