# angelscript

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
| The grammar | `src/angelscript/core/AngelscriptGrammar.hpp` |
| The AST | `src/angelscript/core/ast.hpp` |
| Validation checks | `src/angelscript/core/validation/` |
| Services of your own | `src/angelscript/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/angelscript/cli/main.cpp` |
| LSP features | `src/angelscript/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
class Entity : Base, IUpdatable
{
    private int hp;

    void damage(int amount, int kind = 0)
    {
        hp -= amount;
    }

    int health
    {
        get const property { return hp; }
        set property { hp = value; }
    }
}
```

A class, its base list, a member function with a default parameter value, and
a virtual property with a getter and a setter. `example/declarations.as`,
`statements.as` and `expressions.as` illustrate additional language features.

## The command line

```bash
./build/src/angelscript_cli example/example1.as
./build/src/angelscript_cli --dump example/example1.as   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`angelscript_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
