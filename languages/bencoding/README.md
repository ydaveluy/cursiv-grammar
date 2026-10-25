# bencoding

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
| The grammar | `src/bencoding/core/BencodingGrammar.hpp` |
| The AST | `src/bencoding/core/ast.hpp` |
| Validation checks | `src/bencoding/core/validation/` |
| Services of your own | `src/bencoding/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/bencoding/cli/main.cpp` |
| LSP features | `src/bencoding/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

[Bencoding](https://wiki.theory.org/BitTorrentSpecification#Bencoding) is
BitTorrent's data format: four shapes, no delimiters beyond a length count.

```
d4:infod6:lengthi1024e4:name8:file.txt12:piece lengthi16384e6:pieces0:ee4:urlsl21:http://example.com:80ee
```

A dictionary (`d...e`) whose `info` key holds a nested dictionary
(`length`, `name`, `piece length`, an empty `pieces` string), and whose
`urls` key holds a list (`l...e`) of one string. A string is
`<byte length>:<that many bytes>` — `8:file.txt` is the 8 bytes after the
colon, whatever they are; an integer is `i<decimal>e`.

## The command line

```bash
./build/src/bencoding_cli example/dictionary.torrent
./build/src/bencoding_cli --dump example/dictionary.torrent   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`bencoding_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
