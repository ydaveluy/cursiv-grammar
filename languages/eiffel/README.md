# eiffel

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
| The grammar | `src/eiffel/core/EiffelGrammar.hpp` |
| The AST | `src/eiffel/core/ast.hpp` |
| Validation checks | `src/eiffel/core/validation/` |
| Services of your own | `src/eiffel/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/eiffel/cli/main.cpp` |
| LSP features | `src/eiffel/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```eiffel
note
	description: "100 Doors problem"

class
	APPLICATION

create
	make

feature {NONE} -- Initialization

	make
			-- Main application routine.
		do
			initialize_closed_doors
		end

feature -- Basic Operations

	initialize_closed_doors
		do
			across min_door_count |..| max_door_count as ic_address_list loop
				doors.extend (create {DOOR}.make_closed (ic_address_list.item))
			end
		end

	doors: ARRAYED_LIST [DOOR]

end
```

`example/` contains Rosetta-Code-style Eiffel programs, plus `features.e` and
`inspect.e` for additional language constructs.

## The command line

```bash
./build/src/eiffel_cli example/100_doors/application.e
./build/src/eiffel_cli --dump example/100_doors/application.e   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`eiffel_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
