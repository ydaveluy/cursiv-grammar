# hypertalk

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
| The grammar | `src/hypertalk/core/HypertalkGrammar.hpp` |
| The AST | `src/hypertalk/core/ast.hpp` |
| Validation checks | `src/hypertalk/core/validation/` |
| Services of your own | `src/hypertalk/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/hypertalk/cli/main.cpp` |
| LSP features | `src/hypertalk/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
on mouseUp
  put the name of me into oldName
  repeat while the mouse is down
    if the mouseLoc is within the rect of card button "boundary" then
      set the location of me to the mouseLoc
    end if
  end repeat
  set the name of me to oldName
end mouseUp
```

HyperTalk, HyperCard's scripting language: English-like, line-oriented and
case-insensitive. A document is either a **script** (the handlers and
functions attached to a button, field, card...) or a **scriptlet** (bare
statements, as typed into the message box) — this grammar accepts a free mix
of both under one entry, since cursiv parses from one rule per document.

## The command line

```bash
./build/src/hypertalk_cli example/Script.hypertalk
./build/src/hypertalk_cli --dump example/Script.hypertalk   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`hypertalk_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
