# clu

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
| The grammar | `src/clu/core/CluGrammar.hpp` |
| The AST | `src/clu/core/ast.hpp` |
| Validation checks | `src/clu/core/validation/` |
| Services of your own | `src/clu/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/clu/cli/main.cpp` |
| LSP features | `src/clu/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

CLU, Barbara Liskov's language for data abstraction: a module is one
procedure, iterator or cluster, preceded by top-level constant equates.

```
start_up = proc ()
   own po: stream := stream$primary_output()
   for i: int in int$from_to_by(99, 1, -1) do
      if i = 1 then
         stream$putl(po, int$unparse(i) || " bottle of beer on the wall")
      else
         stream$putl(po, int$unparse(i) || " bottles of beer on the wall")
      end
      end
   end start_up
```

`own` declares a routine-local variable; `type$op(...)` invokes an
operation of a type, the closest CLU comes to a method call. Names are not
cross references here: a call like `stream$putl` is not resolved against a
declaration in the same document (routines and clusters live one to a
module, in separate files), so it stays a plain string rather than a
`reference<>`.

## The command line

```bash
./build/src/clu_cli example/bottles-133.clu
./build/src/clu_cli --dump example/bottles-133.clu   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`clu_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
