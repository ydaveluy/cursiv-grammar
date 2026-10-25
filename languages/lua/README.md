# lua

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
| The grammar | `src/lua/core/LuaGrammar.hpp` |
| The AST | `src/lua/core/ast.hpp` |
| Validation checks | `src/lua/core/validation/` |
| Services of your own | `src/lua/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/lua/cli/main.cpp` |
| LSP features | `src/lua/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```lua
function fact(n)
  if n == 0 then
    return 1
  else
    return n * fact(n - 1)
  end
end

print("enter a number:")
a = io.read("*number")
print(fact(a))
```

A chunk is a block of statements, and a block is what a function body, a
`do`/`while`/`for` body and the document itself all are — one AST class.
`example/` contains Lua programs, including the Lua 5.4 test suite.

## The command line

```bash
./build/src/lua_cli example/factorial.lua
./build/src/lua_cli --dump example/factorial.lua   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`lua_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
