# webidl

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
| The grammar | `src/webidl/core/WebidlGrammar.hpp` |
| The AST | `src/webidl/core/ast.hpp` |
| Validation checks | `src/webidl/core/validation/` |
| Services of your own | `src/webidl/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/webidl/cli/main.cpp` |
| LSP features | `src/webidl/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```webidl
[Exposed=Window]
interface Counter {
  constructor(optional long start = 0);
  attribute long value;
  readonly attribute boolean atLimit;
  undefined increment(optional long by = 1);
};

Counter includes EventTarget;
```

WebIDL (Web IDL) describes the interfaces web APIs expose to script:
interfaces, mixins, callbacks, namespaces, dictionaries and enums, each
optionally decorated with `[ExtendedAttributes]`. Names such as `EventTarget`
above are read as plain text, not resolved: a fragment routinely names
interfaces declared in another `.webidl` file entirely.

## The command line

```bash
./build/src/webidl_cli example/html_link_element.webidl
./build/src/webidl_cli --dump example/html_link_element.webidl   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`webidl_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
