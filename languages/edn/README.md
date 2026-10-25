# edn

A language library built with [cursiv](https://github.com/ydaveluy/cursiv).

## Build

Requires CMake 3.30+, Git and a C++20 compiler with `std::format` and `musttail`
support. Cursiv's CI uses GCC 16 and Clang 22.

```bash
cmake -B build
cmake --build build -j
ctest --test-dir build --output-on-failure
```

The build generates the parser automatically. Rebuild after changing the grammar.

## What to edit

| What you want to change | File |
|---|---|
| The grammar | `src/edn/core/EdnGrammar.hpp` |
| The AST | `src/edn/core/ast.hpp` |
| Validation checks | `src/edn/core/validation/` |
| Services of your own | `src/edn/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/edn/cli/main.cpp` |
| LSP features | `src/edn/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

[edn](https://github.com/edn-format/edn) is Clojure's data notation: nil,
booleans, strings, arbitrary-precision numbers, characters, keywords,
symbols, tagged literals, and four collections.

```clojure
{:name "cursiv"
 :tags #{:parser :peg}
 :aliases {:test {:extra-deps {org.clojure/test.check {:mvn/version "1.0.0"}}}}
 :large 92233720368547758080N}
```

A document is zero or more top-level values — `example/` holds one value per
file as well as documents with several. Every value parses into `edn::ast::Val`,
a `std::variant` of nil, a bool, a string, or a pointer to a `Value` node
(a number, a character, a symbol, a keyword, a tag, or a collection).

## The command line

```bash
./build/src/edn_cli example/deps.edn
./build/src/edn_cli --dump example/deps.edn   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`edn_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
