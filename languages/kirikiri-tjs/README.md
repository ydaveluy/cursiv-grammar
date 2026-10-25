# kirikiritjs

TJS2, the scripting language of the Kirikiri/KAG visual-novel engine, built
with [cursiv](https://github.com/ydaveluy/cursiv).

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
| The grammar | `src/kirikiritjs/core/KirikiriTjsGrammar.hpp` |
| The AST | `src/kirikiritjs/core/ast.hpp` |
| Validation checks | `src/kirikiritjs/core/validation/` |
| Services of your own | `src/kirikiritjs/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/kirikiritjs/cli/main.cpp` |
| LSP features | `src/kirikiritjs/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```tjs
class Greeter extends Object
{
  var name = "world";

  function Greeter(aName)
  {
    if (aName isvalid) name = aName;
  }

  function sayHello()
  {
    return "Hello, " + name + "!";
  }

  property loud
  {
    getter { return name.toUpperCase() + "!!!"; }
  }
}

var g = new Greeter("Kirikiri");
Debug.message(g.sayHello());
for (var i = 0; i < 3; i++) Debug.message(i);
```

A TJS2 script is a flat sequence of statements: `class` bodies are ordinary
blocks of statements, not a separate member grammar, and `property` adds a
getter/setter pair beside a name a plain field would otherwise take.

## The command line

```bash
./build/src/kirikiritjs_cli example/test_scripts/test_class.tjs
./build/src/kirikiritjs_cli --dump example/test_scripts/test_class.tjs   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`kirikiritjs_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
