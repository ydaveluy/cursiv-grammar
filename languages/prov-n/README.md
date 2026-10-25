# provn

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
| The grammar | `src/provn/core/ProvNGrammar.hpp` |
| The AST | `src/provn/core/ast.hpp` |
| Validation checks | `src/provn/core/validation/` |
| Services of your own | `src/provn/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/provn/cli/main.cpp` |
| LSP features | `src/provn/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

PROV-N is the W3C provenance notation: a document is a flat log of typed
relations between entities, activities and agents, each named by a qualified
name and dated with an optional timestamp.

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

Every relation may carry its own identifier (`id; ...`) before its arguments,
and a `[attribute=value, ...]` tail; a `-` stands for an argument left
unspecified. `entity`, `activity` and `agent` declare a thing; the other
relations connect things already declared, by name.

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
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
