# rfc822

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
| The grammar | `src/rfc822/core/Rfc822Grammar.hpp` |
| The AST | `src/rfc822/core/ast.hpp` |
| Validation checks | `src/rfc822/core/validation/` |
| Services of your own | `src/rfc822/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/rfc822/cli/main.cpp` |
| LSP features | `src/rfc822/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
very.common@example.com
```

An address is a mailbox — a plain `localpart@domain`, or a phrase naming a
route-addressed one, `"Some Name"<@relay:user@domain>` — or a named group of
mailboxes, `Friends:Alice<a@x.com>Bob<b@x.com>;`. A local part or a domain
splits on `.` into words; a word is either an unquoted run of text or a
`'single-quoted'` one; a domain segment is a plain name or a bracketed
`[domain-literal]`.

## The command line

```bash
./build/src/rfc822_cli example/example1.rfc822
./build/src/rfc822_cli --dump example/example1.rfc822   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`rfc822_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
