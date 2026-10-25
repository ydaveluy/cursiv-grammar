# desktopentry

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
| The grammar | `src/desktopentry/core/DesktopEntryGrammar.hpp` |
| The AST | `src/desktopentry/core/ast.hpp` |
| Validation checks | `src/desktopentry/core/validation/` |
| Services of your own | `src/desktopentry/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/desktopentry/cli/main.cpp` |
| LSP features | `src/desktopentry/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

A [freedesktop.org Desktop Entry](https://specifications.freedesktop.org/desktop-entry-spec/latest/):
groups of `key=value` lines, a locale qualifier in brackets, `;`-separated
lists, and `#` comments.

```
[Desktop Entry]
Type=Application
Name=Htop
Name[fr]=Htop
Icon=htop
Exec=htop
Terminal=true
Categories=System;Monitor;ConsoleOnly;
```

Each group becomes a `Group`, each line an `Entry`; a value is a string, a
number or a boolean, and a key may carry a `Locale` (language, country,
encoding, modifier).

## The command line

```bash
./build/src/desktopentry_cli example/htop.desktop
./build/src/desktopentry_cli --dump example/htop.desktop   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`desktopentry_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
