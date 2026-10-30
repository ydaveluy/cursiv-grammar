# aterm

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
| The grammar | `src/aterm/core/AtermGrammar.hpp` |
| The AST | `src/aterm/core/ast.hpp` |
| Validation checks | `src/aterm/core/validation/` |
| Services of your own | `src/aterm/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/aterm/cli/main.cpp` |
| LSP features | `src/aterm/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

The port covers the plain textual ASCII format and legacy ToolBus `:type`
and `?` annotations described by the CWI ATerm library. Function names and
numbers keep raw spelling; `core/Value.hpp` supplies the C-style quoted-name
interpretation. Binary/blob formats and shared TAF abbreviations are outside
the scope. The CWI C and Java readers have different numeric and escaping
behavior (Java also adds long integers); this port's signed32-bit integer
validation and C-style escaping do not claim Java reader equivalence.

## The sample

```
exam(<appl(<term>,9)>,<int>,<str>)
```

An [ATerm](https://homepages.cwi.nl/~daybuild/daily-books/technology/aterm-guide/aterm-guide.html)
document is one term: a function application (`f(a,b)`, or just `f` for zero
arguments), a number, a list (`[a,b]`), or a placeholder (`<a>`) marking a
hole to fill in later. Any term may carry a trailing `{...}` annotation.

## The command line

```bash
./build/src/aterm_cli example/example2.aterm
./build/src/aterm_cli --dump example/example2.aterm   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`aterm_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
