# xquery

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
| The grammar | `src/xquery/core/XQueryGrammar.hpp` |
| The AST | `src/xquery/core/ast.hpp` |
| Validation checks | `src/xquery/core/validation/` |
| Services of your own | `src/xquery/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/xquery/cli/main.cpp` |
| LSP features | `src/xquery/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```xquery
xquery version "4.0";
declare namespace ex = "http://example.com";
declare variable $base as xs:string := "http://example.com/";
declare function ex:greet($name as xs:string) as xs:string {
    concat("Hello, ", $name, "!")
};
ex:greet("World")
```

A module's prolog declares namespaces, variables and functions; the query body
after it is the expression the document evaluates. `example/` contains XQuery
4 modules with FLWOR expressions, direct and computed constructors, the type
system, switch/typeswitch, try/catch, and the XQuery Update Facility.

## The command line

```bash
./build/src/xquery_cli example/module.xq
./build/src/xquery_cli --dump example/module.xq   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`xquery_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
