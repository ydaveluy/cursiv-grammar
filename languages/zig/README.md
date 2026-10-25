# zig

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
| The grammar | `src/zig/core/ZigGrammar.hpp` |
| The AST | `src/zig/core/ast.hpp` |
| Validation checks | `src/zig/core/validation/` |
| Services of your own | `src/zig/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/zig/cli/main.cpp` |
| LSP features | `src/zig/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```zig
pub fn main() !void {
    const dbg = @import("std").debug;

    var doors = [_]bool{false} ** 101;
    var pass: u8 = 1;

    while (pass <= 100) : (pass += 1) {
        doors[pass - 1] = !doors[pass - 1];
    }

    for (doors, 0..) |open, num|
        if (open)
            dbg.print("Door {d} is open.\n", .{num});
}
```

Zig syntax: declarations, containers (`struct`/`enum`/`union`/`opaque`), and
the full expression and statement grammar (`if`/`while`/`for`/`switch`, labels
and payloads, pointer and slice types, `asm`).

## The command line

```bash
./build/src/zig_cli example/f2.zig
./build/src/zig_cli --dump example/f2.zig   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`zig_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
