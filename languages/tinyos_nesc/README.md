# tinyosnesc

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
| The grammar | `src/tinyosnesc/core/TinyosNescGrammar.hpp` |
| The AST | `src/tinyosnesc/core/ast.hpp` |
| Validation checks | `src/tinyosnesc/core/validation/` |
| Services of your own | `src/tinyosnesc/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/tinyosnesc/cli/main.cpp` |
| LSP features | `src/tinyosnesc/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
module BlinkC @safe()
{
  uses interface Timer<TMilli> as Timer0;
  uses interface Leds;
  uses interface Boot;
}
implementation
{
  event void Boot.booted()
  {
    call Timer0.startPeriodic( 250 );
  }

  event void Timer0.fired()
  {
    call Leds.led0Toggle();
  }
}
```

nesC ([TinyOS](https://github.com/tinyos)) wires components (`module`s and
`configuration`s) through the interfaces they `use` and `provide`; a
`configuration`'s `implementation` lists `components` and wires them with
`->`/`<-`. A module's body is event/task/command declarations and ordinary
C-like statements, kept as text.

## The command line

```bash
./build/src/tinyosnesc_cli example/Blink/BlinkC.nc
./build/src/tinyosnesc_cli --dump example/Blink/BlinkC.nc   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`tinyosnesc_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
