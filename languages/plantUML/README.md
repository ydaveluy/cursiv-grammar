# plantuml

PlantUML class-diagram syntax, built with
[cursiv](https://github.com/ydaveluy/cursiv).

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
| The grammar | `src/plantuml/core/PlantUMLGrammar.hpp` |
| The AST | `src/plantuml/core/ast.hpp` |
| Validation checks | `src/plantuml/core/validation/` |
| Services of your own | `src/plantuml/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/plantuml/cli/main.cpp` |
| LSP features | `src/plantuml/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
@startuml
abstract class Person {
name : String
age : Real
isAdult()  : Boolean
}

Person <|-- Employee

class Employee {
salary : Real
}

@enduml
```

A document is a diagram between `@startuml` and `@enduml`: class and enum
declarations (with an optional stereotype, `extends`, and a body of raw
text), associations drawn with an arrow of `-.|><o*^` symbols and an
optional `"detail"` on either side and a `: name` after it, and associative
classes written `(A, B) .. C` or `C .. (A, B)`.

## The command line

```bash
./build/src/plantuml_cli example/x7.plantuml
./build/src/plantuml_cli --dump example/x7.plantuml   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`plantuml_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
