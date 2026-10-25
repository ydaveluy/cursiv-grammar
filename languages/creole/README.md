# creole

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
| The grammar | `src/creole/core/CreoleGrammar.hpp` |
| The AST | `src/creole/core/ast.hpp` |
| Validation checks | `src/creole/core/validation/` |
| Services of your own | `src/creole/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/creole/cli/main.cpp` |
| LSP features | `src/creole/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

[Creole](http://www.wikicreole.org/) is a wiki markup language: a document is
a sequence of lines, each a run of markup items.

```
= Top-level heading =

You can make things **bold** or //italic//, link to [[internal links]] or
[[http://www.wikicreole.org|external links]], and force a line\\break.

* an unordered item
# a numbered item

|=Name|=Age|
|Alice|30|

----
```

Bold and italics nest and close on the fly; a heading's level and a list
item's level are the run of `=` or `*`/`#` that opens it.

## The command line

```bash
./build/src/creole_cli example/complete.creole
./build/src/creole_cli --dump example/complete.creole   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`creole_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
