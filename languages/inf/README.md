# inf

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
| The grammar | `src/inf/core/InfGrammar.hpp` |
| The AST | `src/inf/core/ast.hpp` |
| Validation checks | `src/inf/core/validation/` |
| Services of your own | `src/inf/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/inf/cli/main.cpp` |
| LSP features | `src/inf/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
[Manufacturer]
%Provider%=DeviceList,NTamd64

[Device_Install.AddReg]
HKR,,PortSubClass,,05
```

A [Windows INF](https://en.wikipedia.org/wiki/INF_file) document is sections
of lines. A section opens with a `[name]` header; a line is one or two
comma-separated lists of values split by `=` — the second list is absent on
a bare directive line like `HKR,,PortSubClass,,05`, and a comma with
nothing before the next one (or the line's end) is an elided, empty item.

## The command line

```bash
./build/src/inf_cli example/example3.inf
./build/src/inf_cli --dump example/example3.inf   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`inf_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
