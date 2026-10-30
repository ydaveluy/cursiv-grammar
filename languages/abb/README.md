# abb

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
| The grammar | `src/abb/core/AbbGrammar.hpp` |
| The AST | `src/abb/core/ast.hpp` |
| Validation checks | `src/abb/core/validation/` |
| Services of your own | `src/abb/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/abb/cli/main.cpp` |
| LSP features | `src/abb/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
module robdata(sysmodule)
	PERS tooldata tMain:=[TRUE,[[0.0,0.0,700.0],[1.0,0.0,0.0,0.0]],
		[0.0,[5.0,0,95.5],[1,0,0,0],0.228,0.094,0]];

	PROC ConfigOn()
		ConfJ\On;
		ConfL\On;
	ENDPROC
endmodule
```

An ABB RAPID module (`.sys`): a `MODULE`/`ENDMODULE` block holding records,
`PERS`/`VAR`/`CONST` declarations, procedures and functions. The statement
subset includes calls (`ConfJ\On`), assignments, returns, `IF` and `WHILE`.
Expressions currently contain literal values, aggregates and references;
general operator expressions, `FOR`, `TEST`, traps and recovery handlers
are outside this port's current subset. Numeric lexemes are preserved.

Strings follow the RobotWare 7 RAPID kernel: quotes and backslashes are
doubled and `\HH` character codes are decoded. Character codes above ASCII
produce UTF-8. Legacy single-quoted character and binary/hexadecimal numeric
forms remain accepted as port extensions.

## The command line

```bash
./build/src/abb_cli example/robdata.sys
./build/src/abb_cli --dump example/robdata.sys   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`abb_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
