# scala

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
| The grammar | `src/scala/core/ScalaGrammar.hpp` |
| The AST | `src/scala/core/ast.hpp` |
| Validation checks | `src/scala/core/validation/` |
| Services of your own | `src/scala/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/scala/cli/main.cpp` |
| LSP features | `src/scala/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```scala
package com.example.features

case class Circle(radius: Double) extends Shape {
  def area: Double = 3.14159 * radius * radius
}

object Shapes {
  def describe(shape: Shape): String = shape match {
    case Circle(r) if r > 0 => "circle"
    case _ => "unknown"
  }
}
```

Scala 2 syntax: declarations, types, pattern matching, for-comprehensions, and
the expression grammar all parse, but nothing is resolved -- names (imports,
types, `Circle`, `Shape`) are text, not cross references, since most of what
they name lives outside the document.

## The command line

```bash
./build/src/scala_cli example/class.scala
./build/src/scala_cli --dump example/class.scala   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`scala_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
