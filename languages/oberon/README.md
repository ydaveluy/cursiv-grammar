# oberon

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
| The grammar | `src/oberon/core/OberonGrammar.hpp` |
| The AST | `src/oberon/core/ast.hpp` |
| Validation checks | `src/oberon/core/validation/` |
| Services of your own | `src/oberon/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/oberon/cli/main.cpp` |
| LSP features | `src/oberon/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
MODULE Hello;
  IMPORT Oberon, Texts;
  VAR W: Texts.Writer;

  PROCEDURE World*;
  BEGIN
    Texts.WriteString(W, "Hello World!");
    Texts.WriteLn(W);
    Texts.Append(Oberon.Log, W.buf);
  END World;

BEGIN
  Texts.OpenWriter(W);
END Hello.
```

A module: an optional `IMPORT` list, `CONST`/`TYPE`/`VAR` sections and
procedure declarations, then an optional `BEGIN` statement sequence closing
on `END` and the module's own name again. This port is syntax-only: names
are plain `string_view` text, not cross references, since Oberon's module
system resolves across separately compiled files this port does not see.

## The command line

```bash
./build/src/oberon_cli example/hello.mod
./build/src/oberon_cli --dump example/hello.mod   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`oberon_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
