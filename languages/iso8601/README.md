# iso8601

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
| The grammar | `src/iso8601/core/Iso8601Grammar.hpp` |
| The AST | `src/iso8601/core/ast.hpp` |
| Validation checks | `src/iso8601/core/validation/` |
| Services of your own | `src/iso8601/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/iso8601/cli/main.cpp` |
| LSP features | `src/iso8601/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
1985-04-12T23:20:00
```

A date, a time, a datetime, a duration (`P1Y2M15DT12H`) or a recurring
interval (`R2/P1Y6M`) — the grammar tries them in that order, which is what
makes a bare `15` read as an hour rather than a century. `example/` contains
ISO 8601 inputs.

## The command line

```bash
./build/src/iso8601_cli example/b-1-3-cdlt-e.iso8601
./build/src/iso8601_cli --dump example/b-1-3-cdlt-e.iso8601   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`iso8601_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
