# muparser

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
| The grammar | `src/muparser/core/MuparserGrammar.hpp` |
| The AST | `src/muparser/core/ast.hpp` |
| Validation checks | `src/muparser/core/validation/` |
| Services of your own | `src/muparser/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/muparser/cli/main.cpp` |
| LSP features | `src/muparser/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
max(sign(20),rint(3.73),abs(-9),sqrt(16))
(5>3?true:false)?log(5)+sin(0.9+3)^4+log2(6)-asinh(0.2):(2-1)
sum(sin(0.1*_pi),cos(0.2),tan(0.2),asin(0.2),acos(0.2),atan(0.2))
min(sinh(0.2), cosh(0.2), tanh(0.2), asinh(0.2), acosh(1.2), atanh(0.2))
sin(_pi/6)
```

A document is one or more expressions: numbers, `_e`/`_pi`, `true`/`false`, a
bare name as a variable reference, `sin(x)`-style single-argument functions,
`max(a,b,...)`-style variadic ones, the usual arithmetic and comparison
operators, `&&`/`||`, `? :`, and `x = ...`/`x += ...` assignment — reachable
anywhere an operand is, not only at the top of an expression.

## The command line

```bash
./build/src/muparser_cli example/example1.muparser
./build/src/muparser_cli --dump example/example1.muparser   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`muparser_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
