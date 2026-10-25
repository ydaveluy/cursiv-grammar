# racketbsl

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
| The grammar | `src/racketbsl/core/RacketBslGrammar.hpp` |
| The AST | `src/racketbsl/core/ast.hpp` |
| Validation checks | `src/racketbsl/core/validation/` |
| Services of your own | `src/racketbsl/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/racketbsl/cli/main.cpp` |
| LSP features | `src/racketbsl/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```racket
#lang htdp/bsl

(require rackunit)

(define (fahrenheit->celsius f)
    (* 5/9 (- f 32)))

(check-expect (fahrenheit->celsius 212) 100)
(check-expect (fahrenheit->celsius -40) -40)
```

Racket's Beginning Student Language: definitions, `check-expect` test
cases and `require` are top-level forms, and an expression is s-expression
syntax throughout — no infix, no indentation. `example/constructs.rkt`
carries what this sample does not: `define-struct`, a lambda-named
function, `cond`/`if`/`and`/`or`, every literal kind, every `check-`
form and every `require` form.

## The command line

```bash
./build/src/racketbsl_cli example/temperature-conversion.rkt
./build/src/racketbsl_cli --dump example/temperature-conversion.rkt   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`racketbsl_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
