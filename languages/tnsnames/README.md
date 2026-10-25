# tnsnames

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
| The grammar | `src/tnsnames/core/TnsnamesGrammar.hpp` |
| The AST | `src/tnsnames/core/ast.hpp` |
| Validation checks | `src/tnsnames/core/validation/` |
| Services of your own | `src/tnsnames/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/tnsnames/cli/main.cpp` |
| LSP features | `src/tnsnames/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

An Oracle `tnsnames.ora`: aliases mapped to a connect descriptor or a
listener address, keywords case-insensitive, `#` comments.

```
LSNR_WILMA =
  (ADDRESS = (PROTOCOL = IPC)(KEY = LISTENER))

orcl =
  (DESCRIPTION =
    (ADDRESS = (PROTOCOL = TCP)(HOST = dbhost)(PORT = 1521))
    (CONNECT_DATA = (SERVICE_NAME = orcl.example.com))
  )
```

The first entry is a listener: a bare address, no `CONNECT_DATA`. The second
is a service: the same `DESCRIPTION` shape, but `CONNECT_DATA` is what tells
them apart -- both build the same AST class, `connectData` null on the
former.

## The command line

```bash
./build/src/tnsnames_cli example/tnsnames.test.ora
./build/src/tnsnames_cli --dump example/tnsnames.test.ora   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`tnsnames_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
