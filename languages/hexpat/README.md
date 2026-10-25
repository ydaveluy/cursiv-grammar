# hexpat

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
| The grammar | `src/hexpat/core/HexpatGrammar.hpp` |
| The AST | `src/hexpat/core/ast.hpp` |
| Validation checks | `src/hexpat/core/validation/` |
| Services of your own | `src/hexpat/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/hexpat/cli/main.cpp` |
| LSP features | `src/hexpat/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

hexpat is [ImHex](https://github.com/WerWolv/ImHex)'s pattern language for
describing binary file formats: structs, unions, bitfields and enums laid
over a byte stream, with C-like expressions and functions to compute offsets
and sizes.

```
struct map_item {
    u16 type;
    u16 unused;
    u32 size;
    u32 offset;
};

struct map_list {
    u32 size;
    map_item list[size];
};

map_list list @ header.map_off;
```

A field's size may be another field read earlier in the same struct
(`list[size]`), and `@` places a value at a byte offset instead of the next
free one -- both ordinary reads of the AST, not special cases.

## The command line

```bash
./build/src/hexpat_cli example/dex.hexpat
./build/src/hexpat_cli --dump example/dex.hexpat   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`hexpat_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
