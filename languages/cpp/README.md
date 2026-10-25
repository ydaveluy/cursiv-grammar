# cpp

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
| The grammar | `src/cpp/core/CppGrammar.hpp` |
| The AST | `src/cpp/core/ast.hpp` |
| Validation checks | `src/cpp/core/validation/` |
| Services of your own | `src/cpp/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/cpp/cli/main.cpp` |
| LSP features | `src/cpp/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

C++14 syntax: declarations, classes, templates, lambdas and exceptions. There
is no symbol table, so a name is syntactic and the classic ambiguities (a
declaration against an expression, a functional-style cast against a call)
resolve by ordered choice rather than by knowing what a name denotes.

```cpp
class Circle : public Shape {
 public:
  Circle(double r) : Shape(Kind::Circle), radius_(r) {}
  double area() const override { return 3.14159 * radius_ * radius_; }

 private:
  double radius_;
};
```

## The command line

```bash
./build/src/cpp_cli example/test1.cpp
./build/src/cpp_cli --dump example/test1.cpp   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`cpp_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
