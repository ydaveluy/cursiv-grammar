# evmbytecode

A language library built with [cursiv](https://github.com/ydaveluy/cursiv).

This port reads textual disassembler notation, never raw bytecode. Its opcode
set combines historical aliases, legacy EVM and proposal families; it does not
identify a single deployed fork. Immediate syntax is compared with
[EIP-4200](https://eips.ethereum.org/EIPS/eip-4200),
[EIP-4750](https://eips.ethereum.org/EIPS/eip-4750),
[EIP-6206](https://eips.ethereum.org/EIPS/eip-6206),
[EIP-7480](https://eips.ethereum.org/EIPS/eip-7480),
[EIP-7620](https://eips.ethereum.org/EIPS/eip-7620) and
[EIP-8024](https://eips.ethereum.org/EIPS/eip-8024), as consulted 2026-10-28.
The EOF proposals are marked Stagnant and EIP-8024 is under review; accepting
them does not imply deployment. `RETURNCONTRACT` is retained as a historical
alias for the proposal's `RETURNCODE` spelling.

Validation checks selected operand widths and reserved immediate values.
It does not verify EOF containers, jump targets, stack effects, gas, fork
availability or whether an unknown-byte annotation matches an opcode table.
Final truncated PUSH immediates retain the disassembly spelling.

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
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
