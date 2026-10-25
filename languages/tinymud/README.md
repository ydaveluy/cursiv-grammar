# tinymud

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
| The grammar | `src/tinymud/core/TinymudGrammar.hpp` |
| The AST | `src/tinymud/core/ast.hpp` |
| Validation checks | `src/tinymud/core/validation/` |
| Services of your own | `src/tinymud/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/tinymud/cli/main.cpp` |
| LSP features | `src/tinymud/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
@desc me=Igor is a ferret with an evil glint in his eye.
@lock me=me
@fail me=Igor chomps you on the knee with his little sharp teeth.
@ofail me=howls in pain as Igor bites them.
```

A TinyMUD document is a transcript: one `@command` or plain-verb action per
line. `object`, `player`, `name` and free-text fields (a description, a
message) are all the same `STRING` token, whose class includes a space — a
multi-word message is one token, not several.

## The command line

```bash
./build/src/tinymud_cli example/example1.tinymud
./build/src/tinymud_cli --dump example/example1.tinymud   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`tinymud_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
