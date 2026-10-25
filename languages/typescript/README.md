# typescript

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
| The grammar | `src/typescript/core/TypeScriptGrammar.hpp` |
| The AST | `src/typescript/core/ast.hpp` |
| Validation checks | `src/typescript/core/validation/` |
| Services of your own | `src/typescript/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/typescript/cli/main.cpp` |
| LSP features | `src/typescript/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```ts
interface Shape {
  area(): number;
}

class Circle implements Shape {
  constructor(private radius: number) {}
  area(): number {
    return Math.PI * this.radius ** 2;
  }
}

function describe<T extends Shape>(shape: T): string {
  return `area = ${shape.area()}`;
}

console.log(describe(new Circle(2)));
```

A program is a sequence of statements and declarations: variables, functions,
classes, interfaces, type aliases, enums, namespaces, `import`/`export`, the
full expression grammar (arrow functions, destructuring, template literals,
optional chaining, generators, async/await), and TypeScript's own additions --
generics, type annotations, decorators, abstract members, non-null assertions,
`as`/old-style casts.

## The command line

```bash
./build/src/typescript_cli example/Class.ts
./build/src/typescript_cli --dump example/Class.ts   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`typescript_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
