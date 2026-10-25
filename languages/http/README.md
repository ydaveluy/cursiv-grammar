# http

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
| The grammar | `src/http/core/HttpGrammar.hpp` |
| The AST | `src/http/core/ast.hpp` |
| Validation checks | `src/http/core/validation/` |
| Services of your own | `src/http/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/http/cli/main.cpp` |
| LSP features | `src/http/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
POST /url?sa=t&source=web HTTP/1.1
Host: www.google.com.hk
Connection: close
Content-Length: 4
```

An HTTP/1.1 request: a request line (method, target, version) followed by
zero or more header fields, each `name: value` — no message body, and no
status line, matching the subset the reference grammar covers.

## The command line

```bash
./build/src/http_cli example/post.http
./build/src/http_cli --dump example/post.http   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`http_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
