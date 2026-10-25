# memcachedprotocol

A language library built with [cursiv](https://github.com/ydaveluy/cursiv).

## Build

Requires CMake 3.30+, Git and a C++20 compiler with `std::format` and `musttail`
support. Cursiv's CI uses GCC 16 and Clang 22.

```bash
cmake -B build
cmake --build build -j
ctest --test-dir build --output-on-failure
```

The build generates the parser automatically. Rebuild after changing the grammar.

## What to edit

| What you want to change | File |
|---|---|
| The grammar | `src/memcachedprotocol/core/MemcachedProtocolGrammar.hpp` |
| The AST | `src/memcachedprotocol/core/ast.hpp` |
| Validation checks | `src/memcachedprotocol/core/validation/` |
| Services of your own | `src/memcachedprotocol/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/memcachedprotocol/cli/main.cpp` |
| LSP features | `src/memcachedprotocol/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
set greeting 0 0 5
get greeting
delete greeting
quit
```

A document is a run of memcached command lines: storage (`set`, `add`,
`replace`, `append`, `prepend`, `cas`), retrieval (`get`, `gets`), `delete`,
`incr`, `decr`, `stats`, `flush_all`, `version`, `verbosity`, `quit`. The
grammar reads the request line only, as the reference `.g4`'s `command_line`
rule does — not the raw data block a real `set` sends after it.

## The command line

```bash
./build/src/memcachedprotocol_cli example/example1.memcachedprotocol
./build/src/memcachedprotocol_cli --dump example/example1.memcachedprotocol   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`memcachedprotocol_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
