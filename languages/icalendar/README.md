# icalendar

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
| The grammar | `src/icalendar/core/IcalendarGrammar.hpp` |
| The AST | `src/icalendar/core/ast.hpp` |
| Validation checks | `src/icalendar/core/validation/` |
| Services of your own | `src/icalendar/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/icalendar/cli/main.cpp` |
| LSP features | `src/icalendar/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

An iCalendar (RFC 5545) document: a VCALENDAR wrapping one or more
components, each a flat list of `NAME[;PARAM=VALUE...]:VALUE` content
lines. A line that continues onto the next starts it with a single space or
tab -- "folding" -- which the grammar removes on the wire, so a folded value
reads as one unbroken span.

```
BEGIN:VCALENDAR
VERSION:2.0
PRODID:-//xyz Corp//NONSGML PDA Calendar Version 1.0//EN
BEGIN:VEVENT
DTSTAMP:19960704T120000Z
UID:uid1@example.com
DTSTART:19960918T143000Z
SUMMARY:Networld+Interop Conference
END:VEVENT
END:VCALENDAR
```

`DTSTART`'s value is validated as a date or date-time, `SUMMARY`'s as text;
an unrecognised name (`X-WR-CALNAME`, a vendor extension) still parses, its
value read as a raw span. See `example/example1.ics` (the upstream corpus) and `tests/coverage.ics`
(every component kind, folding, lists and periods) for more.

## The command line

```bash
./build/src/icalendar_cli example/example1.ics
./build/src/icalendar_cli --dump example/example1.ics   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`icalendar_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
