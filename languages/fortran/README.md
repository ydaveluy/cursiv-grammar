# fortran

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
| The grammar | `src/fortran/core/FortranGrammar.hpp` |
| The AST | `src/fortran/core/ast.hpp` |
| Validation checks | `src/fortran/core/validation/` |
| Services of your own | `src/fortran/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/fortran/cli/main.cpp` |
| LSP features | `src/fortran/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```fortran
program greet
  character(len=32) :: name
  name = 'world'
  call sub(name)
contains
  subroutine sub(who)
    character(len=*), intent(in) :: who
    print *, 'Hello, ', who, '!'
  end subroutine sub
end program greet
```

A program is a sequence of program units: a main program, functions,
subroutines, modules and block data, each with a specification part and an
executable part. Free source form only: `!` starts a line comment and `&` is a
pure continuation marker, so a statement may already cross lines without
either. Fortran keywords are never reserved — `integer :: if` declares a
variable named `if` — so most identifier positions accept any spelling a
statement keyword could also open on.

## The command line

```bash
./build/src/fortran_cli example/4105.f90
./build/src/fortran_cli --dump example/4105.f90   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`fortran_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
