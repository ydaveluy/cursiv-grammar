# evmbytecode

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
| The grammar | `src/evmbytecode/core/EvmBytecodeGrammar.hpp` |
| The AST | `src/evmbytecode/core/ast.hpp` |
| Validation checks | `src/evmbytecode/core/validation/` |
| Services of your own | `src/evmbytecode/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/evmbytecode/cli/main.cpp` |
| LSP features | `src/evmbytecode/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
PUSH1 0x80
PUSH1 0x40
MSTORE
PUSH1 0x04
CALLDATASIZE
LT
PUSH2 0x004e
JUMPI
'fe'(Unknown Opcode)
```

A document is a flat sequence of EVM opcodes: plain mnemonics (`STOP`,
`MSTORE`, `JUMPI`...), a `PUSHn` with its `n`-byte hex immediate, or a byte
the disassembler could not name, `'xx'(Unknown Opcode)`.

## The command line

```bash
./build/src/evmbytecode_cli example/0xcDA72070E455bb31C7690a170224Ce43623d0B6f.evm
./build/src/evmbytecode_cli --dump example/0xcDA72070E455bb31C7690a170224Ce43623d0B6f.evm   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`evmbytecode_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
