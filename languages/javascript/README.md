# javascript

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
| The grammar | `src/javascript/core/JavaScriptGrammar.hpp` |
| The AST | `src/javascript/core/ast.hpp` |
| Validation checks | `src/javascript/core/validation/` |
| Services of your own | `src/javascript/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/javascript/cli/main.cpp` |
| LSP features | `src/javascript/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```js
class Greeter {
  #name;
  constructor(name) {
    this.#name = name;
  }
  greet() {
    return `Hello, ${this.#name}!`;
  }
}

const alice = new Greeter("Alice");
console.log(alice.greet());
```

A program is a sequence of statements and declarations: variables, functions,
classes, `import`/`export`, control flow, and the full expression grammar
(arrow functions, destructuring, template literals, optional chaining,
generators, async/await).

## The command line

```bash
./build/src/javascript_cli example/Classes.js
./build/src/javascript_cli --dump example/Classes.js   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`javascript_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
