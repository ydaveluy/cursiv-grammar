# yini

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
| The grammar | `src/yini/core/YiniGrammar.hpp` |
| The AST | `src/yini/core/ast.hpp` |
| Validation checks | `src/yini/core/validation/` |
| Services of your own | `src/yini/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/yini/cli/main.cpp` |
| LSP features | `src/yini/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

[YINI](https://github.com/YINI-lang) is an INI-flavoured configuration
format with markers for nested sections, quoted keys, lists, inline objects
and several number bases:

```
^ Preferences

theme = "dark"
volume = 85
recent = ["report.yini", "draft.yini"]

    ^^ Window
    title = 'Sample Window'
    size: 1024, 768
```

A header's marker (`^`, `^^^`, or the shorthand `^7`) is repeated per YINI's
own convention up to six times; the language has no rule for the nesting a
deeper marker implies, so this port keeps sections flat, in document order,
and leaves the marker's own spelling on `Section::markerText` for a reader to
interpret.

## The command line

```bash
./build/src/yini_cli example/Short-1.yini
./build/src/yini_cli --dump example/Short-1.yini   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`yini_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
