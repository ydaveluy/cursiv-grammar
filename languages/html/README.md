# html

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
| The grammar | `src/html/core/HtmlGrammar.hpp` |
| The AST | `src/html/core/ast.hpp` |
| Validation checks | `src/html/core/validation/` |
| Services of your own | `src/html/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/html/cli/main.cpp` |
| LSP features | `src/html/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```html
<!DOCTYPE html>
<html>
<body>
  <div id="myDiv" class="a b">Some <b>text</b>.</div>
  <img src="x.png"/>
</body>
</html>
```

A document is an optional `<?xml ...?>` declaration and `<!...>` doctype
(each a raw span, unparsed further) followed by a mixed sequence of
elements, `<script>`/`<style>` blocks (kept whole, body unparsed), scriptlets
(`<?...?>`, `<%...%>`) and comments. An element is a name, its attributes,
and the same mixed content, recursively — text, CDATA sections, comments and
nested elements. Whitespace-only gaps between tags carry no node; text
touching real content keeps its surrounding whitespace exact. A mismatched
closing tag parses and is a validation error, not a parse failure.

## The command line

```bash
./build/src/html_cli example/html4.html
./build/src/html_cli --dump example/html4.html   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`html_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
