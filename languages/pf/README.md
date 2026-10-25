# pf

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
| The grammar | `src/pf/core/PfGrammar.hpp` |
| The AST | `src/pf/core/ast.hpp` |
| Validation checks | `src/pf/core/validation/` |
| Services of your own | `src/pf/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/pf/cli/main.cpp` |
| LSP features | `src/pf/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
ext_if = "kue0"

table <bruteforce> persist

set skip on lo0
set block-policy drop

block log
pass in quick on $ext_if proto tcp from any to any port ssh \
  flags S/SA keep state (max-src-conn-rate 10/30, overload <bruteforce> flush)
pass out on $ext_if proto { tcp, udp } from any to any keep state
```

pf.conf, OpenBSD's packet-filter configuration language: macros (`$ext_if`),
tables, options and filter rules read top to bottom, last matching `pass`/
`block` deciding — except `quick`, which stops the ruleset there.

## The command line

```bash
./build/src/pf_cli example/example11.pf
./build/src/pf_cli --dump example/example11.pf   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`pf_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
