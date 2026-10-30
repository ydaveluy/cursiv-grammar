# provn

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
| The grammar | `src/provn/core/ProvNGrammar.hpp` |
| The AST | `src/provn/core/ast.hpp` |
| Validation checks | `src/provn/core/validation/` |
| Services of your own | `src/provn/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/provn/cli/main.cpp` |
| LSP features | `src/provn/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

[PROV-N](https://www.w3.org/TR/2013/REC-prov-n-20130430/) is the W3C provenance notation (2013 recommendation).

```
document
  prefix ex <http://example.org/>

  entity(ex:e1, [ex:note="a document"])
  agent(ex:ag1)
  activity(ex:a1, 2020-01-01T00:00:00, -)
  wasGeneratedBy(ex:e1, ex:a1, -)
  wasAssociatedWith(ex:a1, ex:ag1, -)
endDocument
```

## The command line

```bash
./build/src/provn_cli example/example1.provn
./build/src/provn_cli --dump example/example1.provn   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`provn_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
