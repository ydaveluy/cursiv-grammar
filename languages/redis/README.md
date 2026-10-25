# redis

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
| The grammar | `src/redis/core/RedisGrammar.hpp` |
| The AST | `src/redis/core/ast.hpp` |
| Validation checks | `src/redis/core/validation/` |
| Services of your own | `src/redis/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/redis/cli/main.cpp` |
| LSP features | `src/redis/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
SET mykey myval NX EX 60
HSET myhash field1 value1 field2 value2
ZADD myzset GT CH 1 a 2 b
EXPIRE mykey 100 NX
```

Redis command syntax: a keyword, then a positional run of arguments -- keys,
values, decimal counts, and flag words such as `NX` or `WITHSCORES`. Every
command parses to one AST class, `ast::Command`: `name` holds the keyword as
written, `keys` the key-name arguments, `args` everything else, in order, each
either text or a typed integer. Keywords are case-insensitive.

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
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
