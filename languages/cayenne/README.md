# cayenne

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
| The grammar | `src/cayenne/core/CayenneGrammar.hpp` |
| The AST | `src/cayenne/core/ast.hpp` |
| Validation checks | `src/cayenne/core/validation/` |
| Services of your own | `src/cayenne/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/cayenne/cli/main.cpp` |
| LSP features | `src/cayenne/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
struct
  public concrete Unit :: # = data MkUnit | ;

  public concrete id :: (a :: #) -> (x :: a) -> a
    = \(a :: #) -> \(x :: a) -> x;

  private applied :: Unit = id Unit MkUnit;

  public abstract Type :: # = #;
.applied
```

A document is one expression. `#` is the type of types; `data` declares a
type's constructors; `struct`/`sig` hold named definitions/signatures;
application is juxtaposition (`f x`), left-associative, with no call syntax
of its own. See `example/` for a constructor pattern (`nat.cayenne`), a
signature block (`showable.cayenne`) and a bare top-level `case`
(`boolean.cayenne`).

## The command line

```bash
./build/src/cayenne_cli tests/unit.cayenne
./build/src/cayenne_cli --dump tests/unit.cayenne   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`cayenne_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
