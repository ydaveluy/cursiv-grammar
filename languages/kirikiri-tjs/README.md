# kirikiritjs

TJS2, the scripting language of the Kirikiri/KAG visual-novel engine, built
with [cursiv](https://github.com/ydaveluy/cursiv).

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
| The grammar | `src/kirikiritjs/core/KirikiriTjsGrammar.hpp` |
| The AST | `src/kirikiritjs/core/ast.hpp` |
| Validation checks | `src/kirikiritjs/core/validation/` |
| Services of your own | `src/kirikiritjs/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/kirikiritjs/cli/main.cpp` |
| LSP features | `src/kirikiritjs/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

TJS2 scripts contain expressions, statements, classes and properties with getters and setters.

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
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
