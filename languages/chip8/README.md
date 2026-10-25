# chip8

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
| The grammar | `src/chip8/core/Chip8Grammar.hpp` |
| The AST | `src/chip8/core/ast.hpp` |
| Validation checks | `src/chip8/core/validation/` |
| Services of your own | `src/chip8/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/chip8/cli/main.cpp` |
| LSP features | `src/chip8/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
CLS
RET
JP 123
OR V1, V3
```

A CHIP-8 assembly listing: one mnemonic per line, uppercase, operands in
hex with no `0x` prefix — registers `V0`-`VF`, a 12-bit address, an 8-bit
byte, or a bare hex digit. `example/instruction-set.chip8` exercises the
rest of the instruction set.

## The command line

```bash
./build/src/chip8_cli example/example1.chip8
./build/src/chip8_cli --dump example/example1.chip8   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`chip8_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
