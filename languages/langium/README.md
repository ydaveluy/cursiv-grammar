# langium

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
| The grammar | `src/langium/core/LangiumGrammar.hpp` |
| The AST | `src/langium/core/ast.hpp` |
| Validation checks | `src/langium/core/validation/` |
| Services of your own | `src/langium/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/langium/cli/main.cpp` |
| LSP features | `src/langium/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

A `.langium` document declares a Langium grammar: rules, terminals,
interfaces and type aliases. This parses the declaration itself — the
grammar of grammars — not what the declared grammar would go on to accept:

```
grammar Sql

entry SqlFile:
    statements+=Statement*
;

Statement:
    (DefinitionRule | RootLevelSelectStatement) ';'?;

CatalogDefinition:
    'CREATE' (catalogSyntax?='CATALOG' | 'DATABASE') name=Identifier;
```

Rule bodies are ordered choices of sequences (`|`, juxtaposition), with
`?`/`*`/`+` cardinality, named feature assignments (`name=Identifier`,
`items+=Item`, `flag?='KEYWORD'`), cross references (`[Type:Terminal]`),
actions (`{infer Type.feature=current}`) and guarded alternatives
(`<param> ... | <!param> ...`). Terminal rules use their own small EBNF:
character ranges (`'a'..'z'`), negation (`!x`), until (`->`), lookahead
(`(?=x)`), and a regex literal escape hatch (`/[a-z]+/`).

## The command line

```bash
./build/src/langium_cli example/sql.langium
./build/src/langium_cli --dump example/sql.langium   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`langium_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
