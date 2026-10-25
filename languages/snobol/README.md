# snobol

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
| The grammar | `src/snobol/core/SnobolGrammar.hpp` |
| The AST | `src/snobol/core/ast.hpp` |
| Validation checks | `src/snobol/core/validation/` |
| Services of your own | `src/snobol/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/snobol/cli/main.cpp` |
| LSP features | `src/snobol/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
OUTPUT = "What is your name?"
Username = INPUT
Username "J"                                             :S(LOVE)
Username "K"                                             :S(HATE)
MEH       OUTPUT = "Hi, " Username                                 :(END)
LOVE      OUTPUT = "How nice to meet you, " Username               :(END)
HATE      OUTPUT = "Oh. It's you, " Username
END
```

A SNOBOL4 line is `label? subject pattern? (= expression+)? (: transfer)?`:
`subject` names a variable, `pattern` matches against it, `= expr` replaces
it, and `:S(...)`/`:F(...)`/`:(...)` transfer control on success, failure,
or unconditionally.

## The command line

```bash
./build/src/snobol_cli example/hello.sno
./build/src/snobol_cli --dump example/hello.sno   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`snobol_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
