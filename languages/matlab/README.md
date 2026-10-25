# matlab

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
| The grammar | `src/matlab/core/MatlabGrammar.hpp` |
| The AST | `src/matlab/core/ast.hpp` |
| Validation checks | `src/matlab/core/validation/` |
| Services of your own | `src/matlab/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/matlab/cli/main.cpp` |
| LSP features | `src/matlab/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
for i = 1:5
  a = i + 1;
end

if a > 0
  b = [1, 2; 3, 4];
end
```

A statement ends at a comma, a semicolon or a line break -- all three are
interchangeable. Indexing and calling a name share one syntax, `name(args)`,
and `:` alone inside it means the whole range along that dimension: `b(1:2,
:)`.

## The command line

```bash
./build/src/matlab_cli example/example1.matlab
./build/src/matlab_cli --dump example/example1.matlab   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`matlab_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
