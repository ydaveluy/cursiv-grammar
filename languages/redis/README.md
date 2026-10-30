# redis

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
| The grammar | `src/redis/core/RedisGrammar.hpp` |
| The AST | `src/redis/core/ast.hpp` |
| Validation checks | `src/redis/core/validation/` |
| Services of your own | `src/redis/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/redis/cli/main.cpp` |
| LSP features | `src/redis/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

Redis command lines as ordered raw arguments. This is a text-command grammar; it does not parse RESP or execute commands.

```
SET mykey myval NX EX 60
HSET myhash field1 value1 field2 value2
ZADD myzset GT CH 1 a 2 b
EXPIRE mykey 100 NX
```

## The command line

```bash
./build/src/redis_cli example/get.redis
./build/src/redis_cli --dump example/get.redis   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`redis_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
