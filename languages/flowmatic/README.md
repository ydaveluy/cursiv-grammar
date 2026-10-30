# flowmatic

A language library built with [cursiv](https://github.com/ydaveluy/cursiv).

The corpus is a compact FLOW-MATIC-like operation language inspired by the
1958 UNIVAC example. The primary reference located was Sperry Rand's
[U1518 FLOW-MATIC Programming System, 1958](https://www.bitsavers.org/pdf/univac/flow-matic/U1518_FLOW-MATIC_Programming_System_1958.pdf).
Only its indexed introductory text was available during this review: direct
fetches were blocked or timed out. The complete native statement/lexical
specification and a matching compiler edition remain unverified.

The local grammar additionally allows omitted operation periods and `(END)`,
empty operations, case-insensitive words and arithmetic extensions; these are
not asserted to be a universal 1958 standard. Selected arithmetic operand and
GIVING checks are local policies pending fuller native evidence. The AST keeps
source file-designator tokens, unresolved operation labels and `hasEnd`; no
native file linking, execution, arithmetic typing or full data-description
language is implemented. No original compiler/runtime was executed.

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
| The grammar | `src/flowmatic/core/FlowmaticGrammar.hpp` |
| The AST | `src/flowmatic/core/ast.hpp` |
| Validation checks | `src/flowmatic/core/validation/` |
| Services of your own | `src/flowmatic/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/flowmatic/cli/main.cpp` |
| LSP features | `src/flowmatic/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
 (0)  INPUT INVENTORY FILE-A PRICE FILE-B ; OUTPUT PRICED-INV FILE-C UNPRICED-INV
     FILE-D ; HSP D .
 (1)  COMPARE PRODUCT-NO (A) WITH PRODUCT-NO (B) ; IF GREATER GO TO OPERATION 10 ;
     IF EQUAL GO TO OPERATION 5 ; OTHERWISE GO TO OPERATION 2 .
     ...
(17)  STOP . (END)
```

[FLOW-MATIC](https://en.wikipedia.org/wiki/FLOW-MATIC) (1958) is a numbered
sequence of lines: each opens on a `(N)` label and holds one or more
statements — `INPUT`/`OUTPUT` file bindings, `COMPARE`/`MOVE`/`TEST` on a
field within a file letter, `IF`/`OTHERWISE ... GO TO OPERATION N` control
transfer. Canonical examples end each operation with `.` and the program with
`(END)`; this import grammar also accepts their omission.

## The command line

```bash
./build/src/flowmatic_cli example/example1.flowmatic
./build/src/flowmatic_cli --dump example/example1.flowmatic   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`flowmatic_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
