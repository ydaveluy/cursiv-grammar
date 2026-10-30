# awk

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
| The grammar | `src/awk/core/AwkGrammar.hpp` |
| The AST | `src/awk/core/ast.hpp` |
| Validation checks | `src/awk/core/validation/` |
| Services of your own | `src/awk/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/awk/cli/main.cpp` |
| LSP features | `src/awk/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

This library parses GNU Awk 5.4 syntax, including namespaces, indirect calls and typed regexp constants.

## The sample

```awk
BEGIN {
  print "Processing file..."
}
{
	if (NF > 0) {
		gsub(/^[[:space:]]+|[[:space:]]+$/, "", $0)
		$0 = tolower($0)
		split($0, words, /[[:space:]]+/)
		print "Line", NR, "contains", length(words), "words."
	}
}
END {
  print "Finished processing file."
}
```

A program is a list of pattern-action rules: `BEGIN`/`END` run once, a bare
pattern (a regex, an expression, a range) runs its action -- or prints the
record when the action is left out -- for every input line.

## The command line

```bash
./build/src/awk_cli example/count_words.awk
./build/src/awk_cli --dump example/count_words.awk   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`awk_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
