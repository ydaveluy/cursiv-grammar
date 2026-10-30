# atl

A language library built with [cursiv](https://github.com/ydaveluy/cursiv).

ATL describes model transformations, libraries and OCL queries using the classic Eclipse textual syntax.

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
| The grammar | `src/atl/core/AtlGrammar.hpp` |
| The AST | `src/atl/core/ast.hpp` |
| Validation checks | `src/atl/core/validation/` |
| Services of your own | `src/atl/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/atl/cli/main.cpp` |
| LSP features | `src/atl/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
module Families2Persons;
create OUT: Persons from IN: Families;

rule Man {
  from
    s: Families!Member (s.age > 0)
  to
    t: Persons!Male (
      fullName <- s.fullName()
    )
}
```

A document is a `module` (a transformation, matched and called rules over
declared source/target models), a `library` (helpers shared between
modules) or a `query` (one OCL expression). The rule body is the embedded
OCL expression language ATL transformations are written in. The sample
lives at `tests/module.atl`; `example/` is empty because the upstream
corpus ships no files of its own.

## The command line

```bash
./build/src/atl_cli tests/module.atl
./build/src/atl_cli --dump tests/module.atl   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`atl_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
