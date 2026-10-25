# cookie

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
| The grammar | `src/cookie/core/CookieGrammar.hpp` |
| The AST | `src/cookie/core/ast.hpp` |
| Validation checks | `src/cookie/core/validation/` |
| Services of your own | `src/cookie/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/cookie/cli/main.cpp` |
| LSP features | `src/cookie/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
LSID=DQAAEaem_vYg; Path=/accounts; Expires=Wed, 13 Jan 2021 22:23:01 GMT; Secure; HttpOnly
```

An HTTP `Cookie`/`Set-Cookie` header value: semicolon-separated
attribute/value pairs. A value is either bare (letters, digits, `- / _ : ,`
and space, read to the next `;`) or a double-quoted string, and an attribute
may carry no value at all (`Secure`, `HttpOnly`).

## The command line

```bash
./build/src/cookie_cli example/example1.cookie
./build/src/cookie_cli --dump example/example1.cookie   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`cookie_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
