# miniscript

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
| The grammar | `src/miniscript/core/MiniscriptGrammar.hpp` |
| The AST | `src/miniscript/core/ast.hpp` |
| Validation checks | `src/miniscript/core/validation/` |
| Services of your own | `src/miniscript/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/miniscript/cli/main.cpp` |
| LSP features | `src/miniscript/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
or_d(pk(A),and_v(v:pkh(B),older(144)))
```

A Miniscript expression: a small function-call language over the fragments
and wrappers of [BIP 379](https://github.com/bitcoin/bips/blob/master/bip-0379.md).
`pk`/`pkh`/`pk_k`/`pk_h` take a key; `older`/`after` a locktime; `sha256`/
`hash256`/`hash160`/`ripemd160` a hash; `and_v`/`and_b`/`and_n`/`or_b`/`or_c`/
`or_d`/`or_i` two sub-expressions; `andor` three; `thresh` a threshold and a
comma-separated list; `multi`/`multi_a` a threshold and a key list. A run of
`acdtvjnlsu` letters followed by `:` wraps the expression that follows. Key
expressions are opaque tokens — a concrete BIP 380 key, or a symbolic name
such as `A` — validating their shape is left to a downstream analyzer.

## The command line

```bash
./build/src/miniscript_cli example/basic.ms
./build/src/miniscript_cli --dump example/basic.ms   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`miniscript_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
