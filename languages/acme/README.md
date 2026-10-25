# acme

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
| The grammar | `src/acme/core/AcmeGrammar.hpp` |
| The AST | `src/acme/core/ast.hpp` |
| Validation checks | `src/acme/core/validation/` |
| Services of your own | `src/acme/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/acme/cli/main.cpp` |
| LSP features | `src/acme/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

[Acme](http://www.cs.cmu.edu/~acme/index.html) is CMU's architecture
description language: families declare reusable component/connector/port/role
types, systems instantiate them, and design rules (`invariant`, `heuristic`,
`analysis`) state constraints over the resulting graph.

```
family ClientAndServerFam = {
    Component Type ClientT = {
        Port sendRequest : ClientPortT = new ClientPortT;
    }

    Component Type ServerT = {
        Property multiThreaded : boolean << default : boolean = false; >>;
    }

    invariant Forall c1 : component in self.components |
        Forall c2 : component in self.components |
            connected(c1, c2) -> declaresType(c1, ClientT);
}
```

`Port sendRequest : ClientPortT = new ClientPortT;` instantiates a port type
inside a component type; the `invariant` at the end is the design-rule
expression language -- quantifiers, paths and set builders over the
architecture -- that `example/` exercises further (`tests/coverage.acme` is
this port's own document, for constructs the CMU corpus does not reach).

## The command line

```bash
./build/src/acme_cli example/ClientAndServerFam.acme
./build/src/acme_cli --dump example/ClientAndServerFam.acme   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`acme_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
