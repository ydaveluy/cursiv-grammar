# desktopentry

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
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
