# properties

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
| The grammar | `src/properties/core/PropertiesGrammar.hpp` |
| The AST | `src/properties/core/ast.hpp` |
| Validation checks | `src/properties/core/validation/` |
| Services of your own | `src/properties/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/properties/cli/main.cpp` |
| LSP features | `src/properties/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```properties
# a comment
website = https://en.wikipedia.org/
language : English
empty
multiline = This line \
continues
path = c:\\wiki\\templates
```

A row is a comment (`#` or `!` to end of line) or a key/value entry. The
delimiter is `:` or `=`; a key with none has no value. `\` escapes the next
byte, or a line terminator to fold the value onto the next line — kept raw,
undecoded, in the tree.

## The command line

```bash
./build/src/properties_cli example/ebean.properties
./build/src/properties_cli --dump example/ebean.properties   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`properties_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
