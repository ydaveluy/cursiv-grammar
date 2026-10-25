# corundum

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
| The grammar | `src/corundum/core/CorundumGrammar.hpp` |
| The AST | `src/corundum/core/ast.hpp` |
| Validation checks | `src/corundum/core/validation/` |
| Services of your own | `src/corundum/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/corundum/cli/main.cpp` |
| LSP features | `src/corundum/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

Corundum is a small Ruby-like scripting language (variables, arrays, `if`/
`unless`/`while`/`for`, functions, and a `pir` block for inline Parrot IR):

```ruby
def factorial(n)
  if n <= 1
    return 1
  end
  return n * factorial(n - 1)
end

a = factorial(5)
puts a
```

## The command line

```bash
./build/src/corundum_cli example/test.rb
./build/src/corundum_cli --dump example/test.rb   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`corundum_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
