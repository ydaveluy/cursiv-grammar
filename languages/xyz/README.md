# xyz

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
| The grammar | `src/xyz/core/XyzGrammar.hpp` |
| The AST | `src/xyz/core/ast.hpp` |
| Validation checks | `src/xyz/core/validation/` |
| Services of your own | `src/xyz/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/xyz/cli/main.cpp` |
| LSP features | `src/xyz/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

[XYZ](https://en.wikipedia.org/wiki/XYZ_file_format) is a molecular
coordinate format: an atom count, a free-text title line, then one line per
atom.

```
5
methane
C    0.000000    0.000000    0.000000
H   +0.629118    0.629118    0.629118
H   -0.629118   -0.629118    0.629118
H   -0.629118    0.629118   -0.629118
H    0.629118   -0.629118   -0.629118
```

The title line carries free text (`benzene example`, `glucose from 2gbp` in
`example/`). The count is not enforced against the number of atom lines by
the grammar — `XyzValidator` warns when they disagree.

## The command line

```bash
./build/src/xyz_cli example/example1.xyz
./build/src/xyz_cli --dump example/example1.xyz   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`xyz_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
