# metric

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
| The grammar | `src/metric/core/MetricGrammar.hpp` |
| The AST | `src/metric/core/ast.hpp` |
| Validation checks | `src/metric/core/validation/` |
| Services of your own | `src/metric/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/metric/cli/main.cpp` |
| LSP features | `src/metric/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
kg*m/s^2
```

A [metric](https://en.wikipedia.org/wiki/Metric_system) unit-of-measure
expression: measures — an optional SI prefix, a unit, an optional integer
exponent (`kg`, `m`, `s^2`) — chained by `*` and `/`. `m` and `T` each name
both a prefix (milli, tera) and a unit (metre, tesla); what follows decides,
so `mm` is a millimetre and `TT` a teratesla.

## The command line

```bash
./build/src/metric_cli example/kmgs2.metric
./build/src/metric_cli --dump example/kmgs2.metric   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`metric_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
