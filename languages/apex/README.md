# apex

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
| The grammar | `src/apex/core/ApexGrammar.hpp` |
| The AST | `src/apex/core/ast.hpp` |
| Validation checks | `src/apex/core/validation/` |
| Services of your own | `src/apex/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/apex/cli/main.cpp` |
| LSP features | `src/apex/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```apex
public with sharing class GrammarShowcase {
    public Integer value;

    public void run(Object obj) {
        List<Account> accounts = [SELECT Id, Name FROM Account WHERE Name != null];
        for (Account account : accounts) {
            System.debug(account.Name);
        }
        update accounts;
    }
}
```

A `.cls` file is one compilation unit: an optional `package`/`import`
prelude (accepted for fidelity with the reference grammar; real Apex code
never writes one) and one or more type declarations. `[SELECT ...]` is a
SOQL/SOSL query, kept as one opaque span — the port does not parse SOQL
itself, only the Apex around it.

## The command line

```bash
./build/src/apex_cli example/ExampleRunAs.cls
./build/src/apex_cli --dump example/ExampleRunAs.cls   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`apex_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
