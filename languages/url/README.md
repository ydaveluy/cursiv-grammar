# url

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
| The grammar | `src/url/core/UrlGrammar.hpp` |
| The AST | `src/url/core/ast.hpp` |
| Validation checks | `src/url/core/validation/` |
| Services of your own | `src/url/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/url/cli/main.cpp` |
| LSP features | `src/url/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
ftps://user:password@example.com/pub/file.txt
```

A `Url` node: `scheme` ("ftps"), an optional `login` (`user`/`password`), a
`host` (a domain name, an IPv4-shaped name, or a bracketed IPv6 literal), an
optional `port`, an optional `path` (segments plus a trailing-slash flag), a
`query` list of key/value pairs, and an optional `frag`.

## The command line

```bash
./build/src/url_cli example/example10.url
./build/src/url_cli --dump example/example10.url   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`url_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
