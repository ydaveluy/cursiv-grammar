# pegen

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
| The grammar | `src/pegen/core/PegenGrammar.hpp` |
| The AST | `src/pegen/core/ast.hpp` |
| Validation checks | `src/pegen/core/validation/` |
| Services of your own | `src/pegen/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/pegen/cli/main.cpp` |
| LSP features | `src/pegen/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

pegen is CPython's PEG metagrammar: the `.gram` format `Tools/peg_generator`
reads to build the real Python parser. A rule is a pipe-separated list of
alternatives; each alternative a sequence of named, guarded or repeated atoms
followed by an optional C action:

```
start[mod_ty]: a=statements ENDMARKER { a }

statements[asdl_stmt_seq*]: a=statement+ { seq_flatten(a) }

statement[stmt_ty]:
    | 'pass' { make_pass() }
    | a=NAME '=' b=expression { make_assign(a, b) }

expression[expr_ty]:
    | l=expression '+' r=term { make_add(l, r) }
    | term
```

## The command line

```bash
./build/src/pegen_cli example/python-10.gram
./build/src/pegen_cli --dump example/python-10.gram   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`pegen_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
