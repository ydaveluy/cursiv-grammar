# argus

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
| The grammar | `src/argus/core/ArgusGrammar.hpp` |
| The AST | `src/argus/core/ast.hpp` |
| Validation checks | `src/argus/core/validation/` |
| Services of your own | `src/argus/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/argus/cli/main.cpp` |
| LSP features | `src/argus/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

[Argus](https://en.wikipedia.org/wiki/Argus_(programming_language)) is
Barbara Liskov's CLU-family language for distributed guardians. A procedure:

```
Fact = proc(n: int) returns(int)
  if n < 2 then
    return 1
  end
  r: int := Fact(n − 1)
  return n * r
end Fact
```

Sample documents live under `tests/`: `statements.arg`, `expressions.arg`,
`types.arg`, `guardian.arg`, `cluster.arg`, `iterator.arg` and
`equates_generic.arg` each exercise a slice of the language.

## The command line

```bash
./build/src/argus_cli tests/statements.arg
./build/src/argus_cli --dump tests/statements.arg   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`argus_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `tests/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
