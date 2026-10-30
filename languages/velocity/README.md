# velocity

Apache Velocity 2.4.1 templates, built with [cursiv](https://github.com/ydaveluy/cursiv).

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
| The grammar | `src/velocity/core/VelocityGrammar.hpp` |
| The AST | `src/velocity/core/ast.hpp` |
| Validation checks | `src/velocity/core/validation/` |
| Services of your own | `src/velocity/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/velocity/cli/main.cpp` |
| LSP features | `src/velocity/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
<html>
<body>
    #set( $foo = "Velocity" )
Hello $foo World!
</body>
</html>
```

A document is literal text with two kinds of island: `$foo`, `$!{foo.bar}`
read a reference and its property/index/method chain; `#set(...)`,
`#if(...)#end`, `#foreach(...)#end` and the rest of the directives read VTL
control flow. Everything else — the HTML above — is a text atom, unchanged.

## The command line

```bash
./build/src/velocity_cli example/hello_world.vm
./build/src/velocity_cli --dump example/hello_world.vm   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`velocity_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
