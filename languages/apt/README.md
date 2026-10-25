# apt

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
| The grammar | `src/apt/core/AptGrammar.hpp` |
| The AST | `src/apt/core/ast.hpp` |
| Validation checks | `src/apt/core/validation/` |
| Services of your own | `src/apt/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/apt/cli/main.cpp` |
| LSP features | `src/apt/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

One line of an apt sources list: a `deb`/`deb-src` record, optional
`[key=value,...]` options, a URI, a distribution and its components.

```
deb [arch=amd64,signed-by=/etc/apt/keyrings/example.gpg] http://archive.ubuntu.com/ubuntu jammy main restricted
```

A record is one document: `example/` holds one line per file. `#`-prefixed
lines, `deb-src`, and `cdrom::[...]path` URIs are in `example/` too.

## The command line

```bash
./build/src/apt_cli tests/basic.list
./build/src/apt_cli --dump tests/basic.list   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`apt_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
