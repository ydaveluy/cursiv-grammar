# bibtex

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
| The grammar | `src/bibtex/core/BibtexGrammar.hpp` |
| The AST | `src/bibtex/core/ast.hpp` |
| Validation checks | `src/bibtex/core/validation/` |
| Services of your own | `src/bibtex/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/bibtex/cli/main.cpp` |
| LSP features | `src/bibtex/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
@article{ding_bloccess_2023,
	title = {Bloccess: {Enabling} {Fine}-{Grained} {Access} {Control} {Based} on {Blockchain}},
	volume = {31},
	journal = {Journal of Network and Systems Management},
	author = {Ding, Yepeng and Sato, Hiroyuki},
	year = {2023},
	pages = {1--34},
}
```

An entry opens on one of thirteen kinds (`article`, `book`, `inproceedings`,
…), case-insensitive and fused to its leading `@` — `@Article` and `@ARTICLE`
both work, `@ article` does not. Inside, a citekey and one or more
`key = value,` fields, each value a `{brace}`-delimited or `"quote"`-delimited
string, or a bare run of digits.

## The command line

```bash
./build/src/bibtex_cli example/references.bib
./build/src/bibtex_cli --dump example/references.bib   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`bibtex_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
