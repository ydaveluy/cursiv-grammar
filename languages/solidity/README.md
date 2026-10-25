# solidity

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
| The grammar | `src/solidity/core/SolidityGrammar.hpp` |
| The AST | `src/solidity/core/ast.hpp` |
| Validation checks | `src/solidity/core/validation/` |
| Services of your own | `src/solidity/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/solidity/cli/main.cpp` |
| LSP features | `src/solidity/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```solidity
pragma solidity ^0.8.20;

contract Ballot {
    function x(address to) external {
        while (foo[to].delegate != address(0)) {
            to = foo[to].delegate;
        }
    }
}
```

A source unit is a run of pragmas, imports, `using` directives and
declarations (contracts, interfaces, libraries, free functions, structs,
enums, errors, events, constants); a contract's body is the same kind of
list plus constructors, state variables and modifiers. Expressions,
statements and inline-assembly (Yul) blocks nest inside function bodies the
way solc's own grammar has them.

## The command line

```bash
./build/src/solidity_cli example/small_test.sol
./build/src/solidity_cli --dump example/small_test.sol   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`solidity_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
