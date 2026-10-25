# modelica

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
| The grammar | `src/modelica/core/ModelicaGrammar.hpp` |
| The AST | `src/modelica/core/ast.hpp` |
| Validation checks | `src/modelica/core/validation/` |
| Services of your own | `src/modelica/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/modelica/cli/main.cpp` |
| LSP features | `src/modelica/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```modelica
model FirstOrder
  parameter Real c=1 "Time constant";
  Real x "An unknown";
equation
  der(x) = -c*x "A first order differential equation";
end FirstOrder;
```

A `model` is a class: declarations (`parameter Real c`) above `equation`,
equations below. The grammar is syntax-only — no name resolution — so
`reference<>` is not used anywhere in the AST.

## The command line

```bash
./build/src/modelica_cli example/example1.mo
./build/src/modelica_cli --dump example/example1.mo   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`modelica_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
