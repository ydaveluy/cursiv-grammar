# ini

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
| The grammar | `src/ini/core/IniGrammar.hpp` |
| The AST | `src/ini/core/ast.hpp` |
| Services of your own | `src/ini/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/ini/cli/main.cpp` |
| LSP features | `src/ini/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

INI files are read line by line, with sections, comments and key/value entries. IniReader keeps values as strings and preserves duplicate sections and keys in order.

```ini
# Both comment markers are recognised.
; Comment lines have their own nodes in the source AST.

title = "cursiv"
version = 4

[server]
host = "localhost"
port = 8080
tls = true

[server.limits]
read . timeout . ms = 250
write.timeout.ms = -250
```

## The command line

```bash
./build/src/ini_cli example/config.ini
./build/src/ini_cli --dump example/config.ini   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`ini_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
