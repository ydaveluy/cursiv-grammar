# llvmir

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
| The grammar | `src/llvmir/core/LlvmIrGrammar.hpp` |
| The AST | `src/llvmir/core/ast.hpp` |
| Validation checks | `src/llvmir/core/validation/` |
| Services of your own | `src/llvmir/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/llvmir/cli/main.cpp` |
| LSP features | `src/llvmir/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

Textual LLVM IR modules contain globals, functions, instructions, constants and metadata.

```llvm
target triple = "x86_64-pc-linux-gnu"

@.fmt = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1

declare i32 @printf(i8*, ...)

define i32 @main() {
entry:
  %sum = add i32 1, 41
  %s = call i32 (i8*, ...) @printf(i8* getelementptr ([4 x i8], [4 x i8]* @.fmt, i64 0, i64 0), i32 %sum)
  ret i32 0
}
```

## The command line

```bash
./build/src/llvmir_cli example/test.ll
./build/src/llvmir_cli --dump example/test.ll   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`llvmir_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
