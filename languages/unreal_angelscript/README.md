# unrealangelscript

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
| The grammar | `src/unrealangelscript/core/UnrealAngelscriptGrammar.hpp` |
| The AST | `src/unrealangelscript/core/ast.hpp` |
| Validation checks | `src/unrealangelscript/core/validation/` |
| Services of your own | `src/unrealangelscript/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/unrealangelscript/cli/main.cpp` |
| LSP features | `src/unrealangelscript/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
UCLASS(Abstract)
class ABaseThing
{
	UPROPERTY(EditAnywhere, Category = "Stats")
	float Speed = 1.0f;

	UFUNCTION(BlueprintCallable)
	void TakeDamage(int32 Amount)
	{
		Speed -= Amount;
	}
};
```

`UCLASS`/`USTRUCT`/`UPROPERTY`/`UFUNCTION` are Unreal's reflection
annotations on a class, a struct, a property or a function; the grammar is
otherwise a scripting dialect of C++ -- classes, enums, namespaces,
operators, lambdas, `Cast<T>(...)` in place of a C-style cast.

## The command line

```bash
./build/src/unrealangelscript_cli example/Example_Actor.as
./build/src/unrealangelscript_cli --dump example/Example_Actor.as   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`unrealangelscript_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
