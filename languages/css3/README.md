# css3

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
| The grammar | `src/css3/core/Css3Grammar.hpp` |
| The AST | `src/css3/core/ast.hpp` |
| Validation checks | `src/css3/core/validation/` |
| Services of your own | `src/css3/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/css3/cli/main.cpp` |
| LSP features | `src/css3/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```css
@import url("fonts.css");

body {
  font-family: Arial, Helvetica, sans-serif;
}

.button:hover,
.button:focus {
  color: #fff;
  background: rgba(0, 0, 0, .5);
}

@media only screen and (min-width: 768px) {
  .button { padding: calc(1em + 2px); }
}
```

A stylesheet is a list of statements: at-rules (`@import`, `@media`, ...) and
rulesets (a selector group and its declarations). Selectors, at-rule
preludes and declaration values all share one component-value shape — a
number, a string, a function call, a hex color, and so on.

## The command line

```bash
./build/src/css3_cli example/example1.css
./build/src/css3_cli --dump example/example1.css   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`css3_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
