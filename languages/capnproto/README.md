# capnproto

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
| The grammar | `src/capnproto/core/CapnprotoGrammar.hpp` |
| The AST | `src/capnproto/core/ast.hpp` |
| Validation checks | `src/capnproto/core/validation/` |
| Services of your own | `src/capnproto/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/capnproto/cli/main.cpp` |
| LSP features | `src/capnproto/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
@0x9eb32e19f86ee174;

using Cxx = import "/capnp/c++.capnp";
$Cxx.namespace("addressbook");

struct Person {
  id @0 :UInt32;
  name @1 :Text;
  email @2 :Text;
  phones @3 :List(PhoneNumber);

  struct PhoneNumber {
    number @0 :Text;
    type @1 :Type;

    enum Type {
      mobile @0;
      home @1;
      work @2;
    }
  }

  employment :union {
    unemployed @4 :Void;
    employer @5 :Text;
    school @6 :Text;
    selfEmployed @7 :Void;
  }
}

struct AddressBook {
  people @0 :List(Person);
}
```

The Cap'n Proto schema language: structs, interfaces, enums, unions, groups,
generics and const declarations. The parsed schema is represented by a
`Document`.

## The command line

```bash
./build/src/capnproto_cli example/addressbook.capnp
./build/src/capnproto_cli --dump example/addressbook.capnp   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`capnproto_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
