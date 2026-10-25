# r

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
| The grammar | `src/r/core/RGrammar.hpp` |
| The AST | `src/r/core/ast.hpp` |
| Validation checks | `src/r/core/validation/` |
| Services of your own | `src/r/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/r/cli/main.cpp` |
| LSP features | `src/r/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```r
readBool <- function() {
  n <- readline(prompt = "Yes or no? (y/n): ")
  while (!grepl("[YyNn]", n)) {
    n <- readline()
  }
  return(ifelse(n == "y" || n == "Y", TRUE, FALSE))
}
```

R has no statement/expression split: an assignment, an `if`, a `for`, a
function definition and a block are all expressions, and a newline ends a
statement the same way a `;` does, unless what follows cannot continue it —
`Program::body` is a flat list of `Expr`.

## The command line

```bash
./build/src/r_cli example/Complete.R
./build/src/r_cli --dump example/Complete.R   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`r_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
