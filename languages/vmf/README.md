# vmf

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
| The grammar | `src/vmf/core/VmfGrammar.hpp` |
| The AST | `src/vmf/core/ast.hpp` |
| Validation checks | `src/vmf/core/validation/` |
| Services of your own | `src/vmf/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/vmf/cli/main.cpp` |
| LSP features | `src/vmf/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
versioninfo
{
	"editorversion" "400"
	"mapversion" "49"
}
world
{
	"id" "1"
	"classname" "worldspawn"
	solid
	{
		"id" "11"
	}
}
```

A [Valve Map File](https://developer.valvesoftware.com/wiki/KeyValues) is a
tree of key/value pairs: a key's payload is either one bare or quoted string,
or a nested `{...}` block of more pairs — a key repeats instead of holding
several values, which is how `solid`, `side` and `entity` occur many times
at one level.

## The command line

```bash
./build/src/vmf_cli example/example1.vmf
./build/src/vmf_cli --dump example/example1.vmf   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`vmf_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
