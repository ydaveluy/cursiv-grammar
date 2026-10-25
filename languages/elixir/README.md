# elixir

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
| The grammar | `src/elixir/core/ElixirGrammar.hpp` |
| The AST | `src/elixir/core/ast.hpp` |
| Validation checks | `src/elixir/core/validation/` |
| Services of your own | `src/elixir/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/elixir/cli/main.cpp` |
| LSP features | `src/elixir/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```elixir
defmodule Math do
  def sum(a, b) do
    a + b
  end
end

IO.puts(Math.sum(1, 2))
```

A module is a `do ... end` block named by a dotted alias; a function is a
`def`/`defp` head with an optional `when` guard, either form of body. No
cross reference: Elixir names resolve at runtime, not statically, so there is
nothing here for a linker to link.

## The command line

```bash
./build/src/elixir_cli example/list.ex
./build/src/elixir_cli --dump example/list.ex   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`elixir_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
