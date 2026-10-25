# stacktrace

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
| The grammar | `src/stacktrace/core/StacktraceGrammar.hpp` |
| The AST | `src/stacktrace/core/ast.hpp` |
| Validation checks | `src/stacktrace/core/validation/` |
| Services of your own | `src/stacktrace/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/stacktrace/cli/main.cpp` |
| LSP features | `src/stacktrace/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
java.lang.IllegalStateException: invalid state reached during startup
	at com.example.app.Bootstrap.init(Bootstrap.java:88)
	at com.example.app.Bootstrap.start_all(Bootstrap.java:41)
	at com.example.app.Main.main(Main.java:12)
```

A document is one exception: a header line naming the class (and, after a
`:`, its message), the frames it was thrown through, and — recursively — the
exception it was `Caused by:`, if any. `example/nested.stacktrace` shows a
two-level cause chain, `... N more` frames, a native and an unknown-source
frame, and a constructor (`<init>`) frame.

## The command line

```bash
./build/src/stacktrace_cli tests/simple.stacktrace
./build/src/stacktrace_cli --dump tests/simple.stacktrace   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`stacktrace_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
