# v

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
| The grammar | `src/v/core/VGrammar.hpp` |
| The AST | `src/v/core/ast.hpp` |
| Validation checks | `src/v/core/validation/` |
| Services of your own | `src/v/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/v/cli/main.cpp` |
| LSP features | `src/v/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```v
fn main() {
	areas := ['game', 'web', 'tools', 'science', 'systems',
	          'embedded', 'GUI', 'mobile']
	for area in areas {
		println('Hello, $area developers!')
	}
}
```

A module clause, imports, `const`/struct/function declarations,
`if`/`switch`/`for` statements and V's composite literals are covered too --
see `tests/features.v`.

## The command line

```bash
./build/src/v_cli example/helloworld.v
./build/src/v_cli --dump example/helloworld.v   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`v_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
