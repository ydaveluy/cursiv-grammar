# xpath

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
| The grammar | `src/xpath/core/XpathGrammar.hpp` |
| The AST | `src/xpath/core/ast.hpp` |
| Validation checks | `src/xpath/core/validation/` |
| Services of your own | `src/xpath/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/xpath/cli/main.cpp` |
| LSP features | `src/xpath/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
//product[id = 47];
$string => upper-case() => normalize-unicode();
for $x in (1, 2, 3) return $x * 2;
```

Each line is one XPath 3.1 expression: a path (`//product[...]`), an arrow
chain piping a string through two functions, and a `for` expression. The
document is the reference corpus's own test shape — one or more `expr;` —
not a single bare expression, since that is what every corpus file holds.

## The command line

```bash
./build/src/xpath_cli example/expressions_001.xpath
./build/src/xpath_cli --dump example/expressions_001.xpath   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`xpath_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
