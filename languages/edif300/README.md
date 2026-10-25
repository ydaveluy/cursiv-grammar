# edif300

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
| The grammar | `src/edif300/core/Edif300Grammar.hpp` |
| The AST | `src/edif300/core/ast.hpp` |
| Validation checks | `src/edif300/core/validation/` |
| Services of your own | `src/edif300/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/edif300/cli/main.cpp` |
| LSP features | `src/edif300/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

EDIF 3 0 0 is one syntax throughout: a parenthesized keyword followed by its
children, `(keyword arg...)`, nested arbitrarily deep. `example/empty.edf`
(the reference's own minimal-but-complete document) opens like this:

```
(edif EXAMPLE_EMPTY
    (edifVersion 3 0 0)
    (edifHeader
        (edifLevel 1)
        (keywordMap
            (k0KeywordLevel)
        )
        (unitDefinitions
            (unit micrometer 1 (e 1 -6) (meter 1))
        )
        ...
    )
    (library LIB ...)
    (design DESIGN ...)
)
```

Because every one of the format's ~670 named constructs shares that one
shape, the AST mirrors it: one class, `ast::Form` (`keyword` plus an ordered
`args` list), covers all of them — `Form::keyword` says which construct a
node is.

## The command line

```bash
./build/src/edif300_cli example/empty.edf
./build/src/edif300_cli --dump example/empty.edf   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`edif300_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
