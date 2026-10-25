# limbo

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
| The grammar | `src/limbo/core/LimboGrammar.hpp` |
| The AST | `src/limbo/core/ast.hpp` |
| Validation checks | `src/limbo/core/validation/` |
| Services of your own | `src/limbo/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/limbo/cli/main.cpp` |
| LSP features | `src/limbo/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
implement Command;

include "sys.m";
sys: Sys;

init(nil: ref Draw->Context, nil: list of string)
{
	sys = load Sys Sys->PATH;
	sys->print("Hello World!\n");
}
```

A Limbo module `implement`s itself, declares what it `include`s, and defines
its functions -- `init` here takes an unnamed (`nil`) context and an unnamed
argument list, matching the shape every Inferno module's entry point has.

## The command line

```bash
./build/src/limbo_cli example/example1.b
./build/src/limbo_cli --dump example/example1.b   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`limbo_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
