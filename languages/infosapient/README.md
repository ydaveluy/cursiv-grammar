# infosapient

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
| The grammar | `src/infosapient/core/InfosapientGrammar.hpp` |
| The AST | `src/infosapient/core/ast.hpp` |
| Validation checks | `src/infosapient/core/validation/` |
| Services of your own | `src/infosapient/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/infosapient/cli/main.cpp` |
| LSP features | `src/infosapient/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

[Infosapient](http://info-sapient.sourceforge.net/White_Paper/BusinessProcessRules.pdf)
is a fuzzy business-rule language: one named rule, an `if` premise of
attribute clauses joined by fuzzy operators, and a `then`/`else` consequent.

```
OverheatAlarm
if
    temperature s very high and pressure are low
then
    alarm s must 0.9
else
    alarm s slightly 0.1
;
```

An attribute clause reads `subject 's'|'are' [hedges] value`: a subject
(`temperature`), the copula `s` or `are`, zero or more linguistic hedges
(`very`, `must`, `slightly`, ...), and a value -- a number, another
attribute's name, or `increased`/`decreased`. Clauses join with a fuzzy
operator (`and`, `or`, `boundedAnd`, `yagerNot`, ...), and both the whole
premise and an individual clause may be parenthesized.

## The command line

```bash
./build/src/infosapient_cli tests/OverheatAlarm.infosapient
./build/src/infosapient_cli --dump tests/OverheatAlarm.infosapient   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`infosapient_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
