# erlang

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
| The grammar | `src/erlang/core/ErlangGrammar.hpp` |
| The AST | `src/erlang/core/ast.hpp` |
| Validation checks | `src/erlang/core/validation/` |
| Services of your own | `src/erlang/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/erlang/cli/main.cpp` |
| LSP features | `src/erlang/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```erlang
-module(fac).
-export([fac/1]).

-spec fac(integer()) -> integer().

fac(Num) ->
    F = fun F(0) ->
                1;
            F(N) when N >= 0 ->
                N * F(N - 1)
        end,
    F(Num).
```

A module is a sequence of attributes and function declarations, each ending
in `.`. Patterns and expressions share one grammar and one AST here (the
reference itself reads a `case`/`receive` clause's pattern as a plain
`expr`), so a name like `F` above is just an expression node wherever it
appears.

## The command line

```bash
./build/src/erlang_cli example/fac.erl
./build/src/erlang_cli --dump example/fac.erl   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`erlang_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
