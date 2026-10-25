# gdscript

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
| The grammar | `src/gdscript/core/GdscriptGrammar.hpp` |
| The AST | `src/gdscript/core/ast.hpp` |
| Validation checks | `src/gdscript/core/validation/` |
| Services of your own | `src/gdscript/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/gdscript/cli/main.cpp` |
| LSP features | `src/gdscript/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```gdscript
extends Node

class_name Player

var health: int = 100
signal died

func _ready():
	if health <= 0:
		emit_signal("died")

func take_damage(amount):
	health -= amount
	for i in range(3):
		print(i)
```

A class body is declarations (`var`, `const`, `signal`, `enum`, `func`,
nested `class`); a method body is statements, blocks delimited by
indentation the way `INDENT`/`DEDENT` are in the reference grammar.

## The command line

```bash
./build/src/gdscript_cli example/getting_started.gd
./build/src/gdscript_cli --dump example/getting_started.gd   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`gdscript_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
