# antlr

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
| The grammar | `src/antlr/core/AntlrGrammar.hpp` |
| The AST | `src/antlr/core/ast.hpp` |
| Validation checks | `src/antlr/core/validation/` |
| Services of your own | `src/antlr/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/antlr/cli/main.cpp` |
| LSP features | `src/antlr/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

`antlr` parses `.g4` files — the ANTLR v4 meta-grammar itself, lexer, parser
or combined:

```antlr
grammar Smoke;

prog: stat+ EOF;

stat: ID '=' expr ';'   # AssignStat
    | 'print' expr ';'  # PrintStat
    ;

expr: expr '*' expr  # MulExpr
    | expr '+' expr  # AddExpr
    | ID              # IdExpr
    | INT             # IntExpr
    ;

ID: [a-zA-Z_][a-zA-Z_0-9]*;
INT: [0-9]+;
WS: [ \t\r\n]+ -> skip;
```

A grammar declaration, a mix of parser and lexer rules, labeled alternatives
(`# AssignStat`), an operator ladder written directly in EBNF, character
sets, and a lexer command (`-> skip`) — every one of them parses into the
`ast::Grammar` tree: `ast::ParserRule`/`ast::LexerRule` nodes holding
`ast::Alt`s of `ast::Element`s (a name reference, a literal, a nested block,
an inline action, a labeled sub-element), each carrying its own `?`/`*`/`+`
suffix. No names are resolved across `import`s or `tokenVocab`: a `.g4` file
is read on its own, as ANTLR's own tool reads a single grammar unit.

## The command line

```bash
./build/src/antlr_cli example/ANTLRv4Parser.g4
./build/src/antlr_cli --dump example/ANTLRv4Parser.g4   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`antlr_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
