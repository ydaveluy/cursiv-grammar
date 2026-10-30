# angelscript

A language library built with [cursiv](https://github.com/ydaveluy/cursiv).

The syntax follows the pre-2.38 AngelScript language families in the corpus,
compared with [AngelCode's BNF](https://www.angelcode.com/angelscript/sdk/docs/manual/doc_script_bnf.html)
and the [2.36–2.38 release history](https://www.angelcode.com/angelscript/changes.php).
This is a syntax subset: 2.38 `using namespace`, `foreach`, variadic parameters
and template function calls, plus 2.37 `delete`, are not implemented. Host type
registration, overload resolution, engine settings and lvalue/type checks are
outside this port. Empty initializer slots remain available in the CST but are
omitted from the AST. Import, function and accessor `attr` fields are ordered
vectors so repeated attributes retain their spelling.

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
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
