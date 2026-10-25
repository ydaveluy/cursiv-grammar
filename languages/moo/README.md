# moo

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
| The grammar | `src/moo/core/MooGrammar.hpp` |
| The AST | `src/moo/core/ast.hpp` |
| Validation checks | `src/moo/core/validation/` |
| Services of your own | `src/moo/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/moo/cli/main.cpp` |
| LSP features | `src/moo/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
@program hello:run
player:tell("Hello to the world of MOO!");
.
```

A moo document is a sequence of `@`-directives: `@program` attaches a body of
statements to an object's verb, `@property`/`@verb`/`@create` and the rest
declare or configure an object. `player:tell(...)` is a verb call: a
property chain (`player`), then the verb it is asked to run. There is no
cross reference here — object and property names are free text, resolved at
run time by the MOO server this AST would feed, not by this port's linker.

## The command line

```bash
./build/src/moo_cli example/hello.moo
./build/src/moo_cli --dump example/hello.moo   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`moo_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
