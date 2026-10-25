# yara

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
| The grammar | `src/yara/core/YaraGrammar.hpp` |
| The AST | `src/yara/core/ast.hpp` |
| Validation checks | `src/yara/core/validation/` |
| Services of your own | `src/yara/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/yara/cli/main.cpp` |
| LSP features | `src/yara/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
rule _d0
{
 strings:
  $a = { 11 }
  $b = { ?? }
  $f = "text"
 condition:
  1 of them
}
```

A rule names its detection logic; `strings:` declares the patterns it looks
for (plain text, a case wildcard `??`, a masked nibble `0?`), and
`condition:` is the boolean expression that decides a match. `example/`
holds the upstream corpus; `tests/coverage.yar` exercises the
constructs that corpus does not reach (regex string values, the bitwise and
case-insensitive operators, `not`/`or`, and the rarer `in`/`of` forms).

## The command line

```bash
./build/src/yara_cli example/test.yar
./build/src/yara_cli --dump example/test.yar   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`yara_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
