# javacc

A port of [JavaCC](https://javacc.github.io/javacc/)'s own grammar language
(the `.jj`/`.jjt` files a JavaCC or JJTree parser generator reads) built with
[cursiv](https://github.com/ydaveluy/cursiv).

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
| The grammar | `src/javacc/core/JavaccGrammar.hpp` |
| The AST | `src/javacc/core/ast.hpp` |
| Validation checks | `src/javacc/core/validation/` |
| Services of your own | `src/javacc/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/javacc/cli/main.cpp` |
| LSP features | `src/javacc/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
PARSER_BEGIN(Simple1)

public class Simple1 {
  public static void main(String args[]) throws ParseException {
    Simple1 parser = new Simple1(System.in);
    parser.Input();
  }
}

PARSER_END(Simple1)

void Input() :
{}
{
  MatchedBraces() ("\n"|"\r")* <EOF>
}

void MatchedBraces() :
{}
{
  "{" [ MatchedBraces() ] "}"
}
```

An options block, a `PARSER_BEGIN`/`PARSER_END` pair naming a real Java
compilation unit, then productions: a BNF production's body (`Input`,
`MatchedBraces`) is Java-flavoured expansions — literals, references to
other productions, `[...]` for optional, `(...)* ` for repetition — and the
action code inside `{ ... }` is real, structurally parsed Java (or, with
`CPPCODE`, the C++-generation extension's own pointer/template/`const`
syntax), not an opaque block.

## The command line

```bash
./build/src/javacc_cli example/Simple1.jj
./build/src/javacc_cli --dump example/Simple1.jj   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`javacc_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
