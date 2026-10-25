# ttm

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
| The grammar | `src/ttm/core/TtmGrammar.hpp` |
| The AST | `src/ttm/core/ast.hpp` |
| Validation checks | `src/ttm/core/validation/` |
| Services of your own | `src/ttm/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/ttm/cli/main.cpp` |
| LSP features | `src/ttm/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
#<ds;F;abcxxdefyy>
#<ss;F;xx;yy>
#<F;11;22>
```

[TTM](https://en.wikipedia.org/wiki/TTM_(programming_language)) is a 1960s
macro processor: a document is a stream of calls. `#<name;arg;arg>` is an
active call, evaluated where it stands; `##<name;arg;arg>` is neutral, kept
as data. An argument is a nested call, a call wrapped in `<...>` (still a
call, the brackets are pure grouping), or text — a run of letters, digits and
the language's punctuation, or the same wrapped in `<...>` as `<abc>`, both
read as the same kind of text.

## The command line

```bash
./build/src/ttm_cli example/example1.ttm
./build/src/ttm_cli --dump example/example1.ttm   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`ttm_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
