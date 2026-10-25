# amazonstateslanguage

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
| The grammar | `src/amazonstateslanguage/core/AmazonStatesLanguageGrammar.hpp` |
| The AST | `src/amazonstateslanguage/core/ast.hpp` |
| Validation checks | `src/amazonstateslanguage/core/validation/` |
| Services of your own | `src/amazonstateslanguage/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/amazonstateslanguage/cli/main.cpp` |
| LSP features | `src/amazonstateslanguage/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```json
{
  "StartAt": "FailState",
  "States": {
    "FailState": {
      "Type": "Fail",
      "ErrorPath": "States.Format('{}', $.Error)",
      "CausePath": "States.Format('This is a custom error message for {}, caused by {}.', $.Error, $.Cause)"
    }
  }
}
```

An [Amazon States Language](https://states-language.net/spec.html) document
is JSON whose object keys are a fixed vocabulary of state-machine field
names (`"Type"`, `"Resource"`, `"Next"`, `"Choices"`...): the grammar reads
each key as its own token and dispatches on it, the way a `States:`/`Choice:`
DSL would. `Parameters`, `Result`, `ItemSelector` and `ResultSelector` stay
plain JSON — the grammar accepts any shape there, including a `"key.$"`
binding to a JSONPath (`$...`), a context-object path (`$$...`), or an
intrinsic function call.

## The command line

```bash
./build/src/amazonstateslanguage_cli example/choice_state.asl
./build/src/amazonstateslanguage_cli --dump example/choice_state.asl   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`amazonstateslanguage_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
