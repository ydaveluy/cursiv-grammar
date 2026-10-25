# glsl

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
| The grammar | `src/glsl/core/GlslGrammar.hpp` |
| The AST | `src/glsl/core/ast.hpp` |
| Validation checks | `src/glsl/core/validation/` |
| Services of your own | `src/glsl/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/glsl/cli/main.cpp` |
| LSP features | `src/glsl/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```glsl
#version 150
#extension GL_ARB_gpu_shader5 : require

uniform int u1;
uniform int u2;
out vec4 result;

void main()
{
    uint v = 0;
    v = uint(u2) - u1; // implicit conversions
}
```

A shader is a translation unit of declarations, function definitions and
preprocessor directives (`#version`, `#extension`, `#define`, `#if`/`#ifdef`
and friends). This grammar recognises every directive syntactically but does
not run a macro-expansion or conditional-inclusion pass.

## The command line

```bash
./build/src/glsl_cli example/simpleFunctionCall.frag
./build/src/glsl_cli --dump example/simpleFunctionCall.frag   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`glsl_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
