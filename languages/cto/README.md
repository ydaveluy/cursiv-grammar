# cto

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
| The grammar | `src/cto/core/CtoGrammar.hpp` |
| The AST | `src/cto/core/ast.hpp` |
| Validation checks | `src/cto/core/validation/` |
| Services of your own | `src/cto/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/cto/cli/main.cpp` |
| LSP features | `src/cto/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

cto is the Hyperledger Composer Modeling Language: a namespace, imports, and
a handful of class kinds (`asset`, `concept`, `participant`, `transaction`,
`event`) each with typed fields.

```
namespace org.sample.car

import org.sample.owner.Owner

enum CarType {
  o VAN
  o CONVERTIBLE
}

asset Vehicle identified by id {
  o String id default = '0815'
  o Integer [] wheels
  o Boolean damaged default=false
  --> Owner owner
}

asset Car extends Vehicle {
  o String model
  o CarType type
  o Integer year range=[1990,]
}
```

`asset`/`concept`/`participant` declare a class, `identified by`/`extends`
its identity or its parent, `o` a contained field and `-->` a relationship to
another declared asset or participant.

## The command line

```bash
./build/src/cto_cli example/car.cto
./build/src/cto_cli --dump example/car.cto   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`cto_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
