# mdx

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
| The grammar | `src/mdx/core/MdxGrammar.hpp` |
| The AST | `src/mdx/core/ast.hpp` |
| Validation checks | `src/mdx/core/validation/` |
| Services of your own | `src/mdx/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/mdx/cli/main.cpp` |
| LSP features | `src/mdx/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

MDX (MultiDimensional eXpressions) is an OLAP query language — `SELECT`
projects members onto axes, `FROM` names a cube:

```
SELECT
    { [Measures].[Store Sales] } ON COLUMNS,
    { [Date].[2002], [Date].[2003] } ON ROWS
 FROM Sales
 WHERE ( [Store].[USA].[CA] )
```

## The command line

```bash
./build/src/mdx_cli example/example1.mdx
./build/src/mdx_cli --dump example/example1.mdx   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`mdx_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
