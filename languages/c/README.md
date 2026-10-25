# c

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
| The grammar | `src/c/core/CGrammar.hpp` |
| The AST | `src/c/core/ast.hpp` |
| Validation checks | `src/c/core/validation/` |
| Services of your own | `src/c/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/c/cli/main.cpp` |
| LSP features | `src/c/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```c
struct list_el {
   int val;
   struct list_el * next;
};

typedef struct list_el item;

void main() {
   item * curr, * head;
   int i;

   head = NULL;
   for(i=1;i<=10;i++) {
      curr = (item *)malloc(sizeof(item));
      curr->val = i;
      curr->next  = head;
      head = curr;
   }
   ...
}
```

`goto`/labels are the one name this language resolves: a `goto` may jump to a
label anywhere in its enclosing function, however deeply nested in `{ }` --
which is what `example/00199.c` and the custom scope computation in
`src/c/core/references/` are for.

## The command line

```bash
./build/src/c_cli example/ll.c
./build/src/c_cli --dump example/ll.c   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`c_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
