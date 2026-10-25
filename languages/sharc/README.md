# sharc

A language library built with [cursiv](https://github.com/ydaveluy/cursiv).

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
| The grammar | `src/sharc/core/SharcGrammar.hpp` |
| The AST | `src/sharc/core/ast.hpp` |
| Validation checks | `src/sharc/core/validation/` |
| Services of your own | `src/sharc/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/sharc/cli/main.cpp` |
| LSP features | `src/sharc/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

SHARC is a DSP assembly language (Analog Devices SHARC processors):
instructions, register-file operands and assembler directives.

```
.section /pm my_code;

start: r0 = r1 + r2;
       if eq jump start;
       rts;
.endseg;

.var count = 4;
```

`example/` is empty; sample documents live under `tests/`.

## The command line

```bash
./build/src/sharc_cli tests/instructions.dsp
./build/src/sharc_cli --dump tests/instructions.dsp   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`sharc_lsp` speaks LSP over stdio. In VS Code, open this folder and press
**F5**: the task builds the server in the tree of the preset you have selected
-- GCC/Clang presets run the host binary, WASI presets the wasm module -- and a second
window opens with the extension loaded. Point it at `tests/` to use the sample
documents. The preset is read through the CMake Tools extension, which the
workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
