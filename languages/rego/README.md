# rego

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
| The grammar | `src/rego/core/RegoGrammar.hpp` |
| The AST | `src/rego/core/ast.hpp` |
| Validation checks | `src/rego/core/validation/` |
| Services of your own | `src/rego/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/rego/cli/main.cpp` |
| LSP features | `src/rego/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
package example.test

import data.foo as bar

default allow = false

allow {
    bar.enabled
    not bar.denied
}
```

A document is a sequence of packages, imports, rules and bare queries. A rule
head names a value, a partial-set/object key, or nothing at all; each `{...}`
body is a query of literals, each negatable and modifiable with `with ... as
...`; `else` chains an alternative body onto the same rule.

## The command line

```bash
./build/src/rego_cli example/01725.rego
./build/src/rego_cli --dump example/01725.rego   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`rego_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
