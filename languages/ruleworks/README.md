# ruleworks

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
| The grammar | `src/ruleworks/core/RuleworksGrammar.hpp` |
| The AST | `src/ruleworks/core/ast.hpp` |
| Validation checks | `src/ruleworks/core/validation/` |
| Services of your own | `src/ruleworks/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/ruleworks/cli/main.cpp` |
| LSP features | `src/ruleworks/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

RuleWorks (DEC/CPQ's evolutionary step past OPS5) is a forward-chaining,
object-oriented rules language: `object-class` declares a working-memory
shape, `rule` matches a left-hand side of condition elements against it and
fires a right-hand side of actions.

```
(object-class iterator ^count)
(object-class limit ^value)

(on-entry
    (make limit ^value 10)
    (make iterator ^count 1))

(rule increment-rule
    (limit ^value <lim>)
    (iterator ^$id <it> ^count { <num> <= <lim> })
  -->
    (write |   | <num> (crlf))
    (modify <it> ^count (<num> + 1)))
```

`^count` attaches an attribute; `<num>` is a variable bound across the whole
rule; `{ <num> <= <lim> }` is a restriction on an attribute's value. `-->`
separates the conditions from the actions.

## The command line

```bash
./build/src/ruleworks_cli example/count.rul
./build/src/ruleworks_cli --dump example/count.rul   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`ruleworks_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
