# upnp

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
| The grammar | `src/upnp/core/UpnpGrammar.hpp` |
| The AST | `src/upnp/core/ast.hpp` |
| Validation checks | `src/upnp/core/validation/` |
| Services of your own | `src/upnp/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/upnp/cli/main.cpp` |
| LSP features | `src/upnp/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

A [UPnP ContentDirectory search criteria](http://upnp.org/specs/av/UPnP-av-ContentDirectory-v2-Service.pdf)
string: a property test, or two of them joined by `and`/`or`, or `*` for
everything.

```
upnp:class derivedfrom "object.item.audioItem.musicTrack" and (dc:title contains "FindThis")
```

`upnp:class`, `dc:title`, `@id`... are a closed set of property names; the
relational operators (`=`, `!=`, `<`, `<=`, `>`, `>=`, `contains`,
`doesnotcontain`, `derivedfrom`) compare a property against a quoted value,
and `exists` compares it against `true`/`false`. A plain space is the one
significant whitespace character the grammar places explicitly around
properties, operators and `and`/`or` -- a tab, newline or carriage return is
trivia everywhere.

## The command line

```bash
./build/src/upnp_cli example/search1.upnp
./build/src/upnp_cli --dump example/search1.upnp   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`upnp_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
