# abnf

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
| The grammar | `src/abnf/core/AbnfGrammar.hpp` |
| The AST | `src/abnf/core/ast.hpp` |
| Validation checks | `src/abnf/core/validation/` |
| Services of your own | `src/abnf/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/abnf/cli/main.cpp` |
| LSP features | `src/abnf/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```abnf
postal-address   = name-part street zip-part

name-part        = *(personal-part SP) last-name [SP suffix] CRLF
name-part        =/ personal-part CRLF

personal-part    = first-name / (initial ".")
...
```

Augmented BNF for Syntax Specifications (RFC 5234, RFC 7405): a rulename, a
defining `=` or the incremental `=/`, and a right side of alternatives,
concatenations and repetitions over rule references, quoted strings
(`%s"..."` case-sensitive, `%i"..."` case-insensitive), `%b`/`%d`/`%x`
numeric values, groups, options and prose descriptions. Rule references are
plain names, not cross references: an ABNF document commonly names a rule
another file defines (`example/rfc5322.abnf`).

## The command line

```bash
./build/src/abnf_cli example/abnf.abnf
./build/src/abnf_cli --dump example/abnf.abnf   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`abnf_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
