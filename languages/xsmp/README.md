# xsmp

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
| The grammar | `src/xsmp/core/XsmpGrammar.hpp` |
| The AST | `src/xsmp/core/ast.hpp` |
| Qualified names in scope | `src/xsmp/core/XsmpScopeComputation.cpp` |
| Services of your own | `src/xsmp/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/xsmp/cli/main.cpp` |
| LSP features | `src/xsmp/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp`, `tests/recovery/` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
catalogue demo

namespace Demo
{
    /** @uuid 6cdd0a83-0d69-4a9a-84a5-0dbf7f0b1e02 */
    public primitive Float64

    /** @uuid 6cdd0a83-0d69-4a9a-84a5-0dbf7f0b1e03 */
    public enum Mode
    {
        Idle = 0,
        Running = 1
    }

    /** A model with a field, a property, an entry point and two operations. */
    public model Counter
    {
        transient field Float64 count
        property Mode mode
        entrypoint Tick
        def void Reset ()
        def Float64 result Scale (in Float64 factor)
    }
}
```

A catalogue of the ECSS SMP modelling language: namespaces holding types --
primitives, enumerations, structures, classes, exceptions, arrays, interfaces,
models, services and more -- whose members are constants, fields, properties,
operations, associations, containers, references, entry points and event sinks
and sources. Documentation comments carry tagged metadata (`@uuid`), which the
hover shows.

A name is written dotted, `Smp.Publication.IType`: the scope computation
publishes every namespace and type under its qualified name, and under its
type's name an enumeration's literals and the constants a type makes public
(`Type.Leaf`), inside a document and across the workspace alike.

`ecss_smp_2020.xsmpcat` and `ecss_smp_l1.xsmpcat`, beside this README, are two
real ECSS SMP catalogues the tests parse and link (`CORPUS.md`);
`example/broken.xsmpcat` is broken on purpose.

## The command line

```bash
./build/src/xsmp_cli example/demo.xsmpcat
./build/src/xsmp_cli --dump example/demo.xsmpcat   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`xsmp_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
