# protobuf

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
| The grammar | `src/protobuf/core/ProtobufGrammar.hpp` |
| The AST | `src/protobuf/core/ast.hpp` |
| Validation checks | `src/protobuf/core/validation/` |
| Services of your own | `src/protobuf/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/protobuf/cli/main.cpp` |
| LSP features | `src/protobuf/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```protobuf
syntax = "proto3";

message SearchRequest {
  string query = 1;
  int32 page_number = 2;
  int32 result_per_page = 3;
}
```

`syntax = "proto3";` opens every document; a `message` is a record type, and
each field's number (never its declaration order) is what the wire format
uses to identify it.

## The command line

```bash
./build/src/protobuf_cli example/helloworld.proto
./build/src/protobuf_cli --dump example/helloworld.proto   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`protobuf_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
