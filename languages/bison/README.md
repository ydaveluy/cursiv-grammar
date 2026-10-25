# bison

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
| The grammar | `src/bison/core/BisonGrammar.hpp` |
| The AST | `src/bison/core/ast.hpp` |
| Validation checks | `src/bison/core/validation/` |
| Services of your own | `src/bison/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/bison/cli/main.cpp` |
| LSP features | `src/bison/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
%token NUM

%left '+' '-'
%left '*' '/'
%left NEG
%right '^'

%%
exp
    : NUM
    | exp '+' exp
    | '-' exp %prec NEG
    | '(' exp ')'
    ;
%%
```

A Bison/Yacc grammar file: a prologue of declarations (`%token`, precedence),
`%%`, the grammar rules, and an optional `%%`-separated epilogue of raw
target-language code. `example/calc.y` and `example/irc.y` are full worked
examples.

## The command line

```bash
./build/src/bison_cli example/calc.y
./build/src/bison_cli --dump example/calc.y   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`bison_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
