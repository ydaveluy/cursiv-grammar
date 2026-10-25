# tcpheader

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
| The grammar | `src/tcpheader/core/TcpheaderGrammar.hpp` |
| The AST | `src/tcpheader/core/ast.hpp` |
| Validation checks | `src/tcpheader/core/validation/` |
| Services of your own | `src/tcpheader/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/tcpheader/cli/main.cpp` |
| LSP features | `src/tcpheader/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

`example/example1.tcpheader` is 20 raw bytes: a TCP segment header exactly as
it appears on the wire (source port, destination port, sequence number, ack
number, flags, window size, checksum, urgent pointer — no options). There is
no text to read; the grammar just delimits eight fixed-width byte spans.

## The command line

```bash
./build/src/tcpheader_cli example/example1.tcpheader
./build/src/tcpheader_cli --dump example/example1.tcpheader   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`tcpheader_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
