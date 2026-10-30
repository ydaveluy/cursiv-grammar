# powerquery

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
| The grammar | `src/powerquery/core/PowerqueryGrammar.hpp` |
| The AST | `src/powerquery/core/ast.hpp` |
| Validation checks | `src/powerquery/core/validation/` |
| Services of your own | `src/powerquery/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/powerquery/cli/main.cpp` |
| LSP features | `src/powerquery/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
section testSection;

myrecord = [
    func = (arg, optional arg1 as text) => { "foo", 1, false, 7.5}
];

CustomType = type table [
    Ttype = text,
    Ntype = number,
    LType = logical,
    Btype = binary,
    Dtype = datetime
];
```

A Power Query M document is an expression or a section.

## The command line

```bash
./build/src/powerquery_cli example/example.pq
./build/src/powerquery_cli --dump example/example.pq   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`powerquery_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
