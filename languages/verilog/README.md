# verilog

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
| The grammar | `src/verilog/core/VerilogGrammar.hpp` |
| The AST | `src/verilog/core/ast.hpp` |
| Validation checks | `src/verilog/core/validation/` |
| Services of your own | `src/verilog/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/verilog/cli/main.cpp` |
| LSP features | `src/verilog/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```verilog
module gates(input a, input b, output y1, output y2);
  wire w;
  and  (y1, a, b);
  bufif0 (w, a, b);
  not  (y2, w);
endmodule
```

Verilog (IEEE 1364-2005): modules, ports, nets and registers, continuous
assignment, procedural blocks and statements, gate and switch primitives,
generate constructs, functions and tasks, UDPs and specify blocks. A
compiler directive (`` `ifdef ``, `` `define ``, ...) is trivia, as in the
reference grammar: it is recognized and skipped, never expanded.
`example/` holds the corpus (ZipCPU FPGA cores) plus one file per construct
the corpus does not exercise (gates, UDPs, specify blocks, configuration
declarations).

## The command line

```bash
./build/src/verilog_cli example/wbuart.v
./build/src/verilog_cli --dump example/wbuart.v   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`verilog_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
