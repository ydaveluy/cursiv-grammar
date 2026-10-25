# javadoc

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
| The grammar | `src/javadoc/core/JavadocGrammar.hpp` |
| The AST | `src/javadoc/core/ast.hpp` |
| Validation checks | `src/javadoc/core/validation/` |
| Services of your own | `src/javadoc/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/javadoc/cli/main.cpp` |
| LSP features | `src/javadoc/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
/**
 * A description text with {@link Hasher#digest(byte[])} inline tags.
 *
 * @param data the bytes to hash
 * @return the checksum
 */
```

A document is a Javadoc comment's content: a description of text runs and
`{@tag ...}` inline tags, followed by `@tag ...` block tags. The surrounding
`/**`/`*/` markers are optional -- a tool that already stripped them feeds
the content alone.

## The command line

```bash
./build/src/javadoc_cli example/SimpleExample.javadoc
./build/src/javadoc_cli --dump example/SimpleExample.javadoc   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`javadoc_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
