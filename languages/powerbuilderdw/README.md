# powerbuilderdw

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
| The grammar | `src/powerbuilderdw/core/PowerbuilderdwGrammar.hpp` |
| The AST | `src/powerbuilderdw/core/ast.hpp` |
| Validation checks | `src/powerbuilderdw/core/validation/` |
| Services of your own | `src/powerbuilderdw/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/powerbuilderdw/cli/main.cpp` |
| LSP features | `src/powerbuilderdw/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
$PBEXPORTHEADER$D_EXAMPLE.SRD
RELEASE 12.0;
DATAWINDOW(UNITS=0 PROCESSING=0 HTMLDW=NO)
TABLE(COLUMN=(TYPE=CHAR(30) NAME=CUSTOMER_NAME DBNAME="CUSTOMER.NAME")
 RETRIEVE="SELECT NAME FROM CUSTOMER "
 )
```

A Sybase PowerBuilder DataWindow export (`.srd`): a nested property list,
all-uppercase by convention. `RETRIEVE=` carries an embedded PBSELECT string
this grammar does not parse further.

## The command line

```bash
./build/src/powerbuilderdw_cli example/d_example_detail_events.srd
./build/src/powerbuilderdw_cli --dump example/d_example_detail_events.srd   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`powerbuilderdw_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
