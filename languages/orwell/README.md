# orwell

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
| The grammar | `src/orwell/core/OrwellGrammar.hpp` |
| The AST | `src/orwell/core/ast.hpp` |
| Validation checks | `src/orwell/core/validation/` |
| Services of your own | `src/orwell/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/orwell/cli/main.cpp` |
| LSP features | `src/orwell/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

Orwell is a small lazy functional language (Wadler, 1985): equations with
guards, patterns, list comprehensions, and fixity declarations for
user-defined operators.

```
classify n = "negative", if n < 0
           = "zero", if n <= 0
           = "positive", otherwise

squares xs = [x * x | x <- xs ; x > 1]

fact 0 = 1
%else fact n = n, if n > 0
```

A function is a sequence of `pattern = expression` equations; `%else`
introduces another one explicitly, `where` attaches local definitions, and a
comma-guard (`, if ...` / `, otherwise`) picks the branch. `tests/` also
carries type synonyms (`==`), data declarations (`:==`), operator type
signatures (`::`), and fixity declarations (`%left`, `%prefix`, ...), each in
its own small document.

## The command line

```bash
./build/src/orwell_cli example/example1.orwell
./build/src/orwell_cli --dump example/example1.orwell   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`orwell_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
