# restructuredtext

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
| The grammar | `src/restructuredtext/core/RestructuredtextGrammar.hpp` |
| The AST | `src/restructuredtext/core/ast.hpp` |
| Validation checks | `src/restructuredtext/core/validation/` |
| Services of your own | `src/restructuredtext/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/restructuredtext/cli/main.cpp` |
| LSP features | `src/restructuredtext/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```rst
Title
=====

A paragraph with *emphasis*, **strong text**, and an inline `literal`.

* A bullet item.
- Another bullet item.

1. A numbered item.

See target_ for the definition, and _target marks it inline.

.. A comment.
```

Sample documents live under `tests/` (`sections.rst`, `inline.rst`) instead of
`example/`.

## The command line

```bash
./build/src/restructuredtext_cli tests/sections.rst
./build/src/restructuredtext_cli --dump tests/sections.rst   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`restructuredtext_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
