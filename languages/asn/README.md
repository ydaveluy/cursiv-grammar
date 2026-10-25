# asn

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
| The grammar | `src/asn/core/AsnGrammar.hpp` |
| The AST | `src/asn/core/ast.hpp` |
| Validation checks | `src/asn/core/validation/` |
| Services of your own | `src/asn/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/asn/cli/main.cpp` |
| LSP features | `src/asn/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
MyModule DEFINITIONS ::=
BEGIN

MyTypes ::= SEQUENCE {
    myObjectId   OBJECT IDENTIFIER,
    mySeqOf      SEQUENCE OF MyInt,
    myBitString  BIT STRING {
                        muxToken(0),
                        modemToken(1)
                 }
}

MyInt ::= INTEGER (0..65535)

END
```

A module is a named sequence of type and value assignments, `::=` reading as
"is defined as". Names are plain text here, not cross references: an ASN.1
module resolves them against `IMPORTS` and its own assignments, which this
grammar accepts but does not link.

## The command line

```bash
./build/src/asn_cli example/example1.asn
./build/src/asn_cli --dump example/example1.asn   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`asn_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
