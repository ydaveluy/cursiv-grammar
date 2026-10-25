# rust

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
| The grammar | `src/rust/core/RustGrammar.hpp` |
| The AST | `src/rust/core/ast.hpp` |
| Validation checks | `src/rust/core/validation/` |
| Services of your own | `src/rust/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/rust/cli/main.cpp` |
| LSP features | `src/rust/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The samples

`example/` contains real Rust source files: a minimal `hello.rs`, files that
exercise raw strings, or-patterns, open ranges, Unicode identifiers, float
literals butting against tuple indexing, and a large real-world file.

## The command line

```bash
./build/src/rust_cli example/hello.rs
./build/src/rust_cli --dump example/hello.rs   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`rust_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
