# amazonstateslanguageintrinsicfunctions

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
| The grammar | `src/amazonstateslanguageintrinsicfunctions/core/AmazonStatesLanguageIntrinsicFunctionsGrammar.hpp` |
| The AST | `src/amazonstateslanguageintrinsicfunctions/core/ast.hpp` |
| Validation checks | `src/amazonstateslanguageintrinsicfunctions/core/validation/` |
| Services of your own | `src/amazonstateslanguageintrinsicfunctions/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/amazonstateslanguageintrinsicfunctions/cli/main.cpp` |
| LSP features | `src/amazonstateslanguageintrinsicfunctions/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
States.Format('Formatting arguments fst={} and snd={}.', $.FunctionInput.fst, $.FunctionInput.snd)
```

A document is one [Amazon States Language intrinsic function][asl-intrinsics]
call: `States.<Name>(args...)`. An argument is a string, an int, a number, a
`true`/`false`, `null`, a `$`-prefixed JSONPath into the state's input, a
`$$`-prefixed path into the context object, or another call — intrinsics
nest, as `example/intrinsic_states_nested_calls.asl-intrinsic` shows.

[asl-intrinsics]: https://docs.aws.amazon.com/step-functions/latest/dg/amazon-states-language-intrinsic-functions.html

## The command line

```bash
./build/src/amazonstateslanguageintrinsicfunctions_cli example/intrinsic_states_format.asl-intrinsic
./build/src/amazonstateslanguageintrinsicfunctions_cli --dump example/intrinsic_states_format.asl-intrinsic   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`amazonstateslanguageintrinsicfunctions_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
