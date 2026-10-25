# stellaris

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
| The grammar | `src/stellaris/core/StellarisGrammar.hpp` |
| The AST | `src/stellaris/core/ast.hpp` |
| Validation checks | `src/stellaris/core/validation/` |
| Services of your own | `src/stellaris/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/stellaris/cli/main.cpp` |
| LSP features | `src/stellaris/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

A Stellaris document is a tree of key/value pairs -- keys and values are
either a bare scalar, a `.`/`@`/`:`-joined path of scalars, or (values only) a
`{ }` group of more key/value pairs:

```
country_event = {
	id = dragon_misc.1
	hide_window = yes

	trigger = {
		NOT = { has_country_flag = is_under_war }
		num_owned_planets > 5
	}

	immediate = {
		set_country_flag = is_under_war
	}
}
```

There is no cross-reference: a name like `dragon_misc.1` is a plain value,
not a link the linker resolves.

## The command line

```bash
./build/src/stellaris_cli example/ongoing_events.stellaris
./build/src/stellaris_cli --dump example/ongoing_events.stellaris   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`stellaris_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
