# xsdregex

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
| The grammar | `src/xsdregex/core/XsdRegexGrammar.hpp` |
| The AST | `src/xsdregex/core/ast.hpp` |
| Validation checks | `src/xsdregex/core/validation/` |
| Services of your own | `src/xsdregex/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/xsdregex/cli/main.cpp` |
| LSP features | `src/xsdregex/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

XSD's `xsd:pattern` facet regular expressions, [as XML Schema Part 2
defines them](https://www.w3.org/TR/2004/REC-xmlschema-2-20041028/#regexs):
character classes with subtraction, Unicode category and block escapes, and
the usual quantifiers. Every byte is significant — there is no whitespace to
skip.

```
[a-z0-9]+(\.[a-z0-9]+)*@\p{L}+(-\p{L}+)*(\.\p{L}+)+
```

An address-shaped pattern: a dotted local part, then an `@`, then a domain of
Unicode-letter labels joined by dots and hyphens.

## The command line

```bash
./build/src/xsdregex_cli example/example-any.xsdregex
./build/src/xsdregex_cli --dump example/example-any.xsdregex   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`xsdregex_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
