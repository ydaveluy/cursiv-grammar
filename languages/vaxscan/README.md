# vaxscan

VAX SCAN 1.2 source syntax, built with [cursiv](https://github.com/ydaveluy/cursiv).

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
| The grammar | `src/vaxscan/core/VaxscanGrammar.hpp` |
| The AST | `src/vaxscan/core/ast.hpp` |
| Validation checks | `src/vaxscan/core/validation/` |
| Services of your own | `src/vaxscan/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/vaxscan/cli/main.cpp` |
| LSP features | `src/vaxscan/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

VAX SCAN is a block-structured VAX/VMS language for text filters,
translators and preprocessors. A module declares a constant, a procedure,
and calls it:

```
MODULE hanoi;

    CONSTANT disks = 'SDI';

    PROCEDURE hanoi ( number: value integer,
                      begin : value integer,
                      dest  : value integer,
                      inter : value integer );

        IF number = 0
        THEN
            RETURN;
        END IF;

        call hanoi( number-1, begin, inter, dest );

        WRITE ' move ', number,
              ' from ', disks[ begin ],
              ' to   ', disks[ dest ];

        call hanoi( number-1, inter, dest, begin );

    END PROCEDURE;

    PROCEDURE main MAIN (  );

        DECLARE number: integer;

        WRITE 'depth:';
        READ number;

        CALL hanoi( number, 1, 2, 3 );

    END PROCEDURE;

END MODULE;
```

Keywords are case-insensitive, `!` starts a line comment, `/* */` a block
comment, and `'...'` (with `''` for an embedded quote) a string literal.

## The command line

```bash
./build/src/vaxscan_cli example/hanoi.scn
./build/src/vaxscan_cli --dump example/hanoi.scn   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`vaxscan_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
