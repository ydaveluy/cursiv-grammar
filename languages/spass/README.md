# spass

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
| The grammar | `src/spass/core/SpassGrammar.hpp` |
| The AST | `src/spass/core/ast.hpp` |
| Validation checks | `src/spass/core/validation/` |
| Services of your own | `src/spass/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/spass/cli/main.cpp` |
| LSP features | `src/spass/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
begin_problem(Sorts).

list_of_descriptions.
name({* Sorts and Plus *}).
author({* Christoph Weidenbach *}).
status(satisfiable).
description({* Defines plus over successor and zero. *}).
end_of_list.

list_of_symbols.
functions[plus,s,zero].
sorts[even,nat].
end_of_list.

list_of_declarations.
subsort(even,nat).
forall([nat(x)],nat(s(x))).
end_of_list.

list_of_formulae(axioms).
formula(forall([nat(y)],equal(plus(y,zero),y))).
end_of_list.

end_problem.
```

A SPASS problem file (`example/sort_decls.spass`) declares sorts, functions
and predicates, states them as first-order declarations and formulae, and
closes with `end_problem.`. `{* ... *}` is free text: a name, an author, a
comment, read verbatim up to the closing `*}`.

## The command line

```bash
./build/src/spass_cli example/sort_decls.spass
./build/src/spass_cli --dump example/sort_decls.spass   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`spass_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
