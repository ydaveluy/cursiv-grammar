# alloy

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
| The grammar | `src/alloy/core/AlloyGrammar.hpp` |
| The AST | `src/alloy/core/ast.hpp` |
| Validation checks | `src/alloy/core/validation/` |
| Services of your own | `src/alloy/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/alloy/cli/main.cpp` |
| LSP features | `src/alloy/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
sig FSObject { parent: lone Dir }
sig Dir extends FSObject { contents: set FSObject }
sig File extends FSObject { }

fact { all d: Dir, o: d.contents | o.parent = d }
fact { File + Dir = FSObject }

one sig Root extends Dir { } { no parent }

assert acyclic { no d: Dir | d in d.^contents }
check acyclic for 5
```

A model is a sequence of signature, fact, predicate, function, assertion and
command declarations; `sig`, `fact` and `assert` bodies hold relational-logic
expressions, built from quantifiers (`all`, `no`, `some`, `lone`, `one`,
`sum`), the relational operators (`.`, `->`, `&`, `+`, `-`, `++`, `<:`, `:>`,
box join `r[x]`) and the usual comparisons and connectives. Names are plain
text: nothing in this port resolves a sig or field reference across the
document, since an Alloy name may equally point at a signature declared in
another, unopened module.

## The command line

```bash
./build/src/alloy_cli example/fs.als
./build/src/alloy_cli --dump example/fs.als   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`alloy_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
