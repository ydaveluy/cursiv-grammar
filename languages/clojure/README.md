# clojure

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
| The grammar | `src/clojure/core/ClojureGrammar.hpp` |
| The AST | `src/clojure/core/ast.hpp` |
| Validation checks | `src/clojure/core/validation/` |
| Services of your own | `src/clojure/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/clojure/cli/main.cpp` |
| LSP features | `src/clojure/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```clojure
(ns helloworld.core)

(defn -main
  "I can say 'Hello World'."
  []
  (println "Hello, World!"))
```

A document is a sequence of forms: lists, vectors, maps, sets, the scalar
literals (strings, numbers, characters, keywords, symbols) and the reader
macros (`'`, `` ` ``, `~`, `~@`, `^`, `@`, `#{}`, `#()`, `#'`, `#_`, `#^`,
`#tag`, dispatch and regex literals, gensym locals like `x#`).

## The command line

```bash
./build/src/clojure_cli example/example1.clj
./build/src/clojure_cli --dump example/example1.clj   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`clojure_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
