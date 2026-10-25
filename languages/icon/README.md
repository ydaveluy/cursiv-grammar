# icon

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
| The grammar | `src/icon/core/IconGrammar.hpp` |
| The AST | `src/icon/core/ast.hpp` |
| Validation checks | `src/icon/core/validation/` |
| Services of your own | `src/icon/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/icon/cli/main.cpp` |
| LSP features | `src/icon/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
procedure main()
write("hello, world")
end
```

A program is one or more declarations: `link`, `global`, `record` and
`procedure`. A procedure's body is a sequence of expressions, since Icon
makes its control structures (`if`, `while`, `case`, `every`, ...) expressions
too. See `example/features.icn` for a document exercising the rest of the
language — declarations, operator ladder, prefix and postfix forms.

## The command line

```bash
./build/src/icon_cli example/hello.icn
./build/src/icon_cli --dump example/hello.icn   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`icon_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
