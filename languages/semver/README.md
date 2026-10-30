# semver

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
| The grammar | `src/semver/core/SemverGrammar.hpp` |
| The AST | `src/semver/core/ast.hpp` |
| Validation checks | `src/semver/core/validation/` |
| Services of your own | `src/semver/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/semver/cli/main.cpp` |
| LSP features | `src/semver/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
2.0.0-rc.1+build.123
```

A document is one version: a `major.minor.patch` core, an optional
`-`-introduced pre-release list, and an optional `+`-introduced build list.
The [SemVer 2.0](https://semver.org/) core components and dot-separated
identifiers stay as text, preserving integers of any size. Names such as
`rc2` and `beta23` are ordinary identifiers. Validation rejects leading
zeroes in core numbers and numeric pre-release identifiers; build identifiers
may contain them. `example/` contains sample versions.

## The command line

```bash
./build/src/semver_cli example/semver-full.semver
./build/src/semver_cli --dump example/semver-full.semver   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`semver_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
