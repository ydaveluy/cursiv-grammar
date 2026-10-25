# kotlin

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
| The grammar | `src/kotlin/core/KotlinGrammar.hpp` |
| The AST | `src/kotlin/core/ast.hpp` |
| Validation checks | `src/kotlin/core/validation/` |
| Services of your own | `src/kotlin/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/kotlin/cli/main.cpp` |
| LSP features | `src/kotlin/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```kotlin
class C {
    val s: String
        get() = ""

    var t: String
        set(value) { t = value }
        get() = t
}
```

`example/` holds seven documents drawn from the Kotlin grammar's own test
corpus (property getters and setters, generic extension functions, trailing
commas, annotations, anonymous objects, nested type navigation) and three
written for this port, covering a construct no corpus document exercises:
a label on a local declaration, a supertype constructor call's three
`callSuffix` forms, and a member access whose right side is a literal, a
parenthesized expression, `if`/`when`/`try`, or a lambda rather than a plain
name. None declare cross-references — Kotlin resolves names against class
members and imports, which this port does not attempt — but every construct
the grammar accepts still parses into a tree.

## The command line

```bash
./build/src/kotlin_cli example/fix-3208.kt
./build/src/kotlin_cli --dump example/fix-3208.kt   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`kotlin_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
