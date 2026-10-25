# semver

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
`-`-introduced pre-release run, an optional `+`-introduced build run. Beyond
the [semver 2.0](https://semver.org/) core, this grammar recognizes common
pre-release "tags" (`alpha`, `beta`, `rc`, `snapshot`, `dev`, ...) as their
own AST node, so `1.2.3-rc2` and `1.2.3-beta23` carry a comparable `Tag`
rather than opaque text. `example/` contains sample versions;
`all-tags.semver` exercises every tag and its alternate spellings.

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
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
