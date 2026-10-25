# dif

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
| The grammar | `src/dif/core/DifGrammar.hpp` |
| The AST | `src/dif/core/ast.hpp` |
| Validation checks | `src/dif/core/validation/` |
| Services of your own | `src/dif/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/dif/cli/main.cpp` |
| LSP features | `src/dif/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

A [DIF](https://en.wikipedia.org/wiki/Data_Interchange_Format) document is a
fixed four-line header — `TABLE`, `VECTORS`, `TUPLES`, `DATA`, each followed
by a `NUM,NUM` pair and a quoted label — then one or more data items, each a
`NUM,NUM` pair followed by `BOT` (begins a tuple), `EOD` (ends the data
section), a quoted string, or `V` (a numeric value, carried in the pair's
second number):

```
TABLE
0,1
"EXCEL"
VECTORS
0,2
""
TUPLES
0,3
""
DATA
0,0
""
-1,0
BOT
1,0
"Text"
...
-1,0
EOD
```

See `example/example1.dif` and `example/example2.dif` for full documents.

## The command line

```bash
./build/src/dif_cli example/example1.dif
./build/src/dif_cli --dump example/example1.dif   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`dif_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
