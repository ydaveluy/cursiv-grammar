# xml

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
| The grammar | `src/xml/core/XmlGrammar.hpp` |
| The AST | `src/xml/core/ast.hpp` |
| Validation checks | `src/xml/core/validation/` |
| Services of your own | `src/xml/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/xml/cli/main.cpp` |
| LSP features | `src/xml/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```xml
<?xml version="1.0"?>
<catalog>
   <book id="bk101">
      <author>Gambardella, Matthew</author>
      <title>XML Developer's Guide</title>
   </book>
</catalog>
```

A document is an optional declaration (its own node, holding the
`<?xml ...?>` attributes) and a root element. An element is a name, its
attributes, and mixed content — text, entity and character references, CDATA
sections, and nested elements. Comments and processing instructions parse
but are not part of the tree; a mismatched closing tag is a validation
error, not a parse failure.

## The command line

```bash
./build/src/xml_cli example/books.xml
./build/src/xml_cli --dump example/books.xml   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`xml_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
