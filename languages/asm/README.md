# nasm

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
| The grammar | `src/nasm/core/NasmGrammar.hpp` |
| The AST | `src/nasm/core/ast.hpp` |
| Validation checks | `src/nasm/core/validation/` |
| Services of your own | `src/nasm/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/nasm/cli/main.cpp` |
| LSP features | `src/nasm/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```asm
        extern  printf

        SECTION .data
fmt:    db "a=%ld, rax=%ld", 10, 0

        SECTION .text
        global main
main:
        push    rbp
        mov     rax, [a]
        add     rax, 2
        mov     rdi, fmt
        mov     rsi, [a]
        mov     rdx, rax
        mov     rax, 0
        call    printf
        pop     rbp
        mov     rax, 0
        ret

        SECTION .data
a:      dq 5
```

A [NASM](https://www.nasm.us/) x86-64 source: labels, sections, the
mnemonic/operand shape of an instruction, and the `db`/`dq` data declarations.
No cross reference resolves (a label is text, not a `reference<>`), so there
is nothing here for go-to-definition to follow; diagnostics still flag an
unparsed construct.

## The command line

```bash
./build/src/nasm_cli example/hello_64.asm
./build/src/nasm_cli --dump example/hello_64.asm   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`nasm_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
