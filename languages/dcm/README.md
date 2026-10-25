# dcm

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
| The grammar | `src/dcm/core/DcmGrammar.hpp` |
| The AST | `src/dcm/core/ast.hpp` |
| Validation checks | `src/dcm/core/validation/` |
| Services of your own | `src/dcm/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/dcm/cli/main.cpp` |
| LSP features | `src/dcm/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
KONSERVIERUNG_FORMAT 2.0

FESTWERT SCHWELLE_1
LANGNAME "Threshold value one"
VAR MOTORTYP=V8, GETRIEBE=AUTOMATIK
EINHEIT_W "bar"
WERT 1.25
END
```

A DCM file (Bosch's ECU calibration/conservation format) is line-oriented:
a `KONSERVIERUNG_FORMAT 2.0` header, an optional module/function/variant
header, then a body of characteristic values -- scalars (`FESTWERT`),
blocks, curves (`KENNLINIE`), maps (`KENNFELD`) and support-point
distributions, each with optional metadata (`LANGNAME`, `DISPLAYNAME`,
`VAR`, `FUNKTION`).

## The command line

```bash
./build/src/dcm_cli tests/kennwert.dcm
./build/src/dcm_cli --dump tests/kennwert.dcm   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`dcm_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
