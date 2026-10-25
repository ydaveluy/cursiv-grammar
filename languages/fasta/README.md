# fasta

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
| The grammar | `src/fasta/core/FastaGrammar.hpp` |
| The AST | `src/fasta/core/ast.hpp` |
| Validation checks | `src/fasta/core/validation/` |
| Services of your own | `src/fasta/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/fasta/cli/main.cpp` |
| LSP features | `src/fasta/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
>gi|158333233|ref|NC_009925.1| Acaryochloris marina MBIC11017 chromosome, complete genome
ATGCCCTATACCTATGATTATCCGCGCCCTGGTCTTACCGTTGACTGCGTGGTGTTTGGCCTAGACGAAC
AGATCGATCTCAAAGTCCTACTGATTCAGCGCCAGATTCCCCCTTTCCAGCATCAGTGGGCATTACCCGG
;a comment line
ACGTACGTACGTACGTACGT
```

A document is a flat list of sections: a description line (`>`, then one or
more `|`-separated text fields), a sequence line (a run of letters, one per
line), or a comment line (`;`, then free text). Nothing is skipped between
lines — a section is exactly one physical line, EOL included — so a run of
consecutive sequence lines reads as that many adjacent `SequenceLine`
sections, and neither a header nor a fixed order is required.

## The command line

```bash
./build/src/fasta_cli example/NC_009925.fna.fasta
./build/src/fasta_cli --dump example/NC_009925.fna.fasta   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`fasta_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
