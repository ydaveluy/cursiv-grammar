# pcre

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
| The grammar | `src/pcre/core/PcreGrammar.hpp` |
| The AST | `src/pcre/core/ast.hpp` |
| Validation checks | `src/pcre/core/validation/` |
| Services of your own | `src/pcre/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/pcre/cli/main.cpp` |
| LSP features | `src/pcre/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
^[A-Z0-9._%+-]+@[A-Z0-9.-]+\.[A-Z]{2,}\b$
```

A PCRE pattern, as `pcre.org` describes it: anchors (`^`, `$`, `\b`), a
character class (`[A-Z0-9._%+-]`), an escaped literal (`\.`), and a
quantifier (`{2,}`). Every byte is significant — there is no trivia, and a
space in a pattern is a literal to match, not layout.

## The command line

```bash
./build/src/pcre_cli example/example1.pcre
./build/src/pcre_cli --dump example/example1.pcre   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`pcre_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
