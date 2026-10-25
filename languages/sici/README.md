# sici

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
| The grammar | `src/sici/core/SiciGrammar.hpp` |
| The AST | `src/sici/core/ast.hpp` |
| Validation checks | `src/sici/core/validation/` |
| Services of your own | `src/sici/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/sici/cli/main.cpp` |
| LSP features | `src/sici/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
0002-8231(199412)45:10<737:TIODIM>2.3.TX;2-M
```

A [SICI](https://en.wikipedia.org/wiki/Serial_Item_and_Contribution_Identifier)
(ANSI/NISO Z39.56): an ISSN, a chronology in parentheses and an enumeration
identify the issue; `<737:TIODIM>` locates a contribution inside it by page
and a compressed title; zero or more `;`-separated control segments follow.
Whitespace is trivia everywhere in the document.

## The command line

```bash
./build/src/sici_cli example/example1.sici
./build/src/sici_cli --dump example/example1.sici   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`sici_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
