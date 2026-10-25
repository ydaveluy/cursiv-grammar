# lisa

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
| The grammar | `src/lisa/core/LisaGrammar.hpp` |
| The AST | `src/lisa/core/ast.hpp` |
| Validation checks | `src/lisa/core/validation/` |
| Services of your own | `src/lisa/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/lisa/cli/main.cpp` |
| LSP features | `src/lisa/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
declare{
 dfa my_dfa;
 int index;
}

program{
 index = 2+3;
 my_dfa = readfile(dfa,"~/dfa.txt");
 print(my_dfa);
}
```

LISA declares its variables up front, typed `int`, `dfa`, `nfa`, `regex`,
`bool` or `string`; the program block then reads and reassigns them. A
variable is a cross reference the linker resolves against its declaration,
which is what makes rename, go-to-definition and "unresolved name"
diagnostics work at all.

## The command line

```bash
./build/src/lisa_cli example/example1.lisa
./build/src/lisa_cli --dump example/example1.lisa   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`lisa_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
