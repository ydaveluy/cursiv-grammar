# szf

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
| The grammar | `src/szf/core/SzfGrammar.hpp` |
| The AST | `src/szf/core/ast.hpp` |
| Validation checks | `src/szf/core/validation/` |
| Services of your own | `src/szf/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/szf/cli/main.cpp` |
| LSP features | `src/szf/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

szf is a restricted [SFZ](https://sfzformat.com) instrument definition: a
document is zero or more `<group>` headers, each holding `key=value` pairs
and nested `<region>` blocks of their own.

```
<group>
ampeg_attack=0.04 ampeg_release=0.45

<region> sample=trumpet_pp_c4.wav key=c4
<region> sample=trumpet_pp_d4.wav key=d4
```

A value is either an identifier (`trumpet_pp_c4.wav`, `c#4` — letters, digits,
`.`, `_`, `/` and `#`) or a number (`0.04`). `example/` contains two sample
banks.

## The command line

```bash
./build/src/szf_cli example/example1.szf
./build/src/szf_cli --dump example/example1.szf   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`szf_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
