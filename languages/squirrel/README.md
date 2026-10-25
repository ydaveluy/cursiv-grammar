# squirrel

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
| The grammar | `src/squirrel/core/SquirrelGrammar.hpp` |
| The AST | `src/squirrel/core/ast.hpp` |
| Validation checks | `src/squirrel/core/validation/` |
| Services of your own | `src/squirrel/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/squirrel/cli/main.cpp` |
| LSP features | `src/squirrel/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
function Ack(M, N) {
    if (M == 0) return( N + 1 );
    if (N == 0) return( Ack(M - 1, 1) );
    return( Ack(M - 1, Ack(M, (N - 1))) );
}

local n = vargv.len() != 0 ? vargv[0].tointeger() : 1;
print("Ack(3," + n + "):" + Ack(3, n));
```

Squirrel statements end at a `;` or at a newline — both a `Chunk` (a
document) and a `{ }` block hold a plain list of statements, and a class or a
table is a list of `key = value` members.

## The command line

```bash
./build/src/squirrel_cli example/ackermann.nut
./build/src/squirrel_cli --dump example/ackermann.nut   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`squirrel_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
