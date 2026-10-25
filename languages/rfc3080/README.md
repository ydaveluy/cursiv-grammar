# rfc3080

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
| The grammar | `src/rfc3080/core/Rfc3080Grammar.hpp` |
| The AST | `src/rfc3080/core/ast.hpp` |
| Validation checks | `src/rfc3080/core/validation/` |
| Services of your own | `src/rfc3080/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/rfc3080/cli/main.cpp` |
| LSP features | `src/rfc3080/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
MSG 0 1 . 52 120
Content-Type: application/beep+xml

<start number='1'>
   <profile uri='http://iana.org/beep/TLS' />
</start>
END
```

A [BEEP](https://tools.ietf.org/html/rfc3080) frame: a header line naming the
message kind (`MSG`, `RPY`, `ANS`, `ERR`, `NUL`), a channel, a message
number, whether more frames follow (`*`) or this is the last (`.`), a
sequence number and a payload size, then the payload itself, closed by a
line reading `END`. `example/` holds one document per kind.

## The command line

```bash
./build/src/rfc3080_cli tests/msg.beep
./build/src/rfc3080_cli --dump tests/msg.beep   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`rfc3080_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
