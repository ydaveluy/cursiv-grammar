# muddb

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
| The grammar | `src/muddb/core/MuddbGrammar.hpp` |
| The AST | `src/muddb/core/ast.hpp` |
| Validation checks | `src/muddb/core/validation/` |
| Services of your own | `src/muddb/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/muddb/cli/main.cpp` |
| LSP features | `src/muddb/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

A LambdaMOO-style room dump: one record per line, sixteen lines per room,
closed by a marker:

```
#0
Room Zero
You are in Room Zero.  It's very dark here.
-1
1
-1
-1
-1



is briefly visible through the mist.
1
0
0

***END OF DUMP***
```

Each room is `roomno name description location contents exits next key
fail_message succ_message ofail osuccess owner pennies flags password`, one
field per line; a blank line is a field left absent rather than empty or
zero.

## The command line

```bash
./build/src/muddb_cli example/minimal.muddb
./build/src/muddb_cli --dump example/minimal.muddb   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`muddb_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
