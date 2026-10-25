# maml

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
| The grammar | `src/maml/core/MamlGrammar.hpp` |
| The AST | `src/maml/core/ast.hpp` |
| Validation checks | `src/maml/core/validation/` |
| Services of your own | `src/maml/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/maml/cli/main.cpp` |
| LSP features | `src/maml/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

MAML — Minimal Abstract Markup Language — is JSON with comments, unquoted
keys, optional commas and multiline strings:

```
# A basic MAML document
{
  project: "MAML"
  tags: ["minimal", "readable"]  # comma-separated
  langs: [                       # or newline-separated, no commas
    "json"
    "maml"
  ]
  count: 42
  enabled: true
  disabled: false
  nothing: null
}
```

A member is separated from the next by a comma or a newline, never both on
the same line. A key may be bare (`project`), quoted, digit-only or one of
`true`/`false`/`null` spelled as a key. A string may be `"quoted, with
\n escapes"` or a triple-quoted raw string (`"""...."""`) that keeps its
content literally.

## The command line

```bash
./build/src/maml_cli example/basic.maml
./build/src/maml_cli --dump example/basic.maml   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`maml_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
