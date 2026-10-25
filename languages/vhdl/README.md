# vhdl

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
| The grammar | `src/vhdl/core/VhdlGrammar.hpp` |
| The AST | `src/vhdl/core/ast.hpp` |
| Validation checks | `src/vhdl/core/validation/` |
| Services of your own | `src/vhdl/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/vhdl/cli/main.cpp` |
| LSP features | `src/vhdl/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```vhdl
package add_pkg is
  procedure add_unsigned (
    signal a : in  std_logic_vector;
    signal b : in  std_logic_vector;
    signal z : out std_logic_vector
    );
end add_pkg;

package body add_pkg is
  procedure add_unsigned (
    signal a : in  std_logic_vector;
    signal b : in  std_logic_vector;
    signal z : out std_logic_vector
    )
  is
  begin
    z <= std_logic_vector(unsigned(a) + unsigned(b));
  end add_unsigned;
end add_pkg;
```

VHDL is case-insensitive throughout (`ENTITY`, `Entity` and `entity` are the
same keyword) and its comments are `--` to end of line or `/* ... */`
(VHDL-2008). `example/` contains package, entity, architecture, configuration
and context declarations, generate statements and protected types.
`coverage.vhd` demonstrates additional constructs: block statements,
force/release, guarded signals, package and subprogram instantiation, and
groups.

## The command line

```bash
./build/src/vhdl_cli example/add_pkg.vhd
./build/src/vhdl_cli --dump example/add_pkg.vhd   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`vhdl_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
