# ini

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
| The grammar | `src/ini/core/IniGrammar.hpp` |
| The AST | `src/ini/core/ast.hpp` |
| Services of your own | `src/ini/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/ini/cli/main.cpp` |
| LSP features | `src/ini/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```ini
# Both comment markers are recognised.
; A comment line is a blank line whose comment the skipper hid.

title = "cursiv"
version = 4

[server]
host = "localhost"
port = 8080
tls = true

[server.limits]
read . timeout . ms = 250
write.timeout.ms = -250
```

Configuration files, a restricted INI/TOML: sections, dotted keys, and four
value kinds -- string, integer, float, boolean. The record separator is the
newline, not a punctuation mark: a record ends at the line, or at the end of
the input for the last one, and the ignore rule alone is what makes the format
line-oriented -- `tests/FreeFormGrammar.hpp`, the same grammar with `\n` put
back into it, accepts what this one refuses. A dotted key's parts are joined,
so `a . b` and `a.b` are one key; a section header allows no trivia between
its brackets.

`example.ini`, beside this README, is the document the grammar test reads
entry by entry; `example/broken.ini` is broken on purpose.

## The command line

```bash
./build/src/ini_cli example/config.ini
./build/src/ini_cli --dump example/config.ini   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`ini_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
