# sieve

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
| The grammar | `src/sieve/core/SieveGrammar.hpp` |
| The AST | `src/sieve/core/ast.hpp` |
| Validation checks | `src/sieve/core/validation/` |
| Services of your own | `src/sieve/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/sieve/cli/main.cpp` |
| LSP features | `src/sieve/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
require ["fileinto"];

if header :is "Sender" "owner-ietf-mta-filters@imc.org"
    {
    fileinto "filter";
    }
else
    {
    fileinto "personal";
    }
```

A Sieve script (RFC 5228) is a sequence of commands: an action ends in `;`,
a control command opens a `{ }` block of nested commands. `if`/`elsif`/`else`
are not keywords of the grammar — every command name is an identifier, and
what follows (a `;` or a block) is what the grammar reads.

## The command line

```bash
./build/src/sieve_cli example/example1.sieve
./build/src/sieve_cli --dump example/example1.sieve   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`sieve_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
