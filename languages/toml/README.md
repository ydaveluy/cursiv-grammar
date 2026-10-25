# toml

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
| The grammar | `src/toml/core/TomlGrammar.hpp` |
| The AST | `src/toml/core/ast.hpp` |
| Validation checks | `src/toml/core/validation/` |
| Services of your own | `src/toml/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/toml/cli/main.cpp` |
| LSP features | `src/toml/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```toml
title = "TOML Example"

[owner]
name = "Tom Preston-Werner"
dob = 1979-05-27T07:32:00-08:00

[database]
ports = [ 8001, 8001, 8002 ]
enabled = true

[servers.alpha]
ip = "10.0.0.1"
```

A document is a sequence of tables: entries written before any `[header]`
belong to the implicit root table, and each `[key]`/`[[key]]` opens a new run
of entries that lasts until the next header. Values reach every TOML kind --
strings (plain, multi-line, literal), integers in four bases, floats
(`inf`/`nan` included), booleans, the four date/time shapes, arrays and
inline tables.

## The command line

```bash
./build/src/toml_cli example/test.toml
./build/src/toml_cli --dump example/test.toml   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`toml_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
