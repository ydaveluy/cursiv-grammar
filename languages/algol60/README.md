# algol60

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
| The grammar | `src/algol60/core/Algol60Grammar.hpp` |
| The AST | `src/algol60/core/ast.hpp` |
| Validation checks | `src/algol60/core/validation/` |
| Services of your own | `src/algol60/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/algol60/cli/main.cpp` |
| LSP features | `src/algol60/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
begin
  integer procedure SIGMA(x, i, n);
    value n;
    integer x, i, n;
  begin
    integer sum;
    sum:=0;
    for i:=1 step 1 until n do
        sum:=sum+x;
    SIGMA:=sum;
  end;
  integer q;
  printnln(SIGMA(q*2-1, q, 7));
end
```

A procedure with a value parameter and a specifier, a `for`/`step`/`until`
loop, and a call whose argument is itself an expression — `example/` holds
the rest of the ALGOL 60 corpus this port was measured against.

## The command line

```bash
./build/src/algol60_cli example/jensen.a60
./build/src/algol60_cli --dump example/jensen.a60   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`algol60_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
