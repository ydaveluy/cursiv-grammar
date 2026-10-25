# smalltalk

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
| The grammar | `src/smalltalk/core/SmalltalkGrammar.hpp` |
| The AST | `src/smalltalk/core/ast.hpp` |
| Validation checks | `src/smalltalk/core/validation/` |
| Services of your own | `src/smalltalk/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/smalltalk/cli/main.cpp` |
| LSP features | `src/smalltalk/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
| sum items |
sum := 0.
items := #(1 2 3 4 5).
items do: [:each | sum := sum + each].
^sum
```

A script is temp declarations (`|sum items|`), a period-separated run of
message sends, and an optional `^answer`. Every send -- unary (`each
factorial`), binary (`a + b`) and keyword (`items do: [...]`) -- lowers to
one `MessageSend` node: a receiver, a selector split into parts, and the
matching arguments.

## The command line

```bash
./build/src/smalltalk_cli example/helloworld.st
./build/src/smalltalk_cli --dump example/helloworld.st   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`smalltalk_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
