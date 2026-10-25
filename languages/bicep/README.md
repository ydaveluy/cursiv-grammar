# bicep

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
| The grammar | `src/bicep/core/BicepGrammar.hpp` |
| The AST | `src/bicep/core/ast.hpp` |
| Validation checks | `src/bicep/core/validation/` |
| Services of your own | `src/bicep/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/bicep/cli/main.cpp` |
| LSP features | `src/bicep/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

[Bicep](https://learn.microsoft.com/azure/azure-resource-manager/bicep/) is
Azure's declarative deployment DSL: newline-terminated statements, and
string interpolation with `'${...}'`.

```
param storageName string = 'examplestorage'

resource stg 'Microsoft.Storage/storageAccounts@2019-06-01' = {
  name: storageName
  location: 'eastus'
  sku: {
    name: 'Standard_LRS'
  }
}

output endpoint string = stg.properties.primaryEndpoints.blob
```

A `resource`/`module` value may also be a `for` loop building a collection,
optionally guarded by `if (...)`; a `type` declaration may combine ambient
types, object and tuple shapes, and unions with `|`.

## The command line

```bash
./build/src/bicep_cli example/program.bicep
./build/src/bicep_cli --dump example/program.bicep   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`bicep_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
