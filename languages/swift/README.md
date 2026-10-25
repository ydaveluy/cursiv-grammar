# swift

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
| The grammar | `src/swift/core/SwiftGrammar.hpp` |
| The AST | `src/swift/core/ast.hpp` |
| Validation checks | `src/swift/core/validation/` |
| Services of your own | `src/swift/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/swift/cli/main.cpp` |
| LSP features | `src/swift/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

Swift 5.4 syntax. `example/` contains Playground-style examples for control
flow, enumerations and structures, error handling, functions and closures,
generics, objects and classes, protocols and extensions, simple values, a
string extension, and a `Package.swift` manifest.

```swift
struct Point {
  var x = 0.0, y = 0.0
}

func distance(from a: Point, to b: Point) -> Double {
  let dx = a.x - b.x, dy = a.y - b.y
  return (dx * dx + dy * dy).squareRoot()
}
```

## The command line

```bash
./build/src/swift_cli example/SimpleValues.swift
./build/src/swift_cli --dump example/SimpleValues.swift   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`swift_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
