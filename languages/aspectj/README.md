# aspectj

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
| The grammar | `src/aspectj/core/AspectjGrammar.hpp` |
| The AST | `src/aspectj/core/ast.hpp` |
| Validation checks | `src/aspectj/core/validation/` |
| Services of your own | `src/aspectj/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/aspectj/cli/main.cpp` |
| LSP features | `src/aspectj/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```java
package com.example;

public aspect LoggingAspect {

    pointcut serviceCall() : execution(* com.example.Service.*(..));

    before() : serviceCall() {
        System.out.println("entering " + thisJoinPoint);
    }

    Object around() : serviceCall() {
        return proceed();
    }
}
```

Java plus the aspect constructs: aspects and their per-clauses, pointcut
declarations and every primitive pointcut designator, advice
(before/after/around, returning/throwing), inter-type declarations, `declare`
forms, and the annotation-style forms (`@Aspect`, `@Before("...")`,
`@Pointcut("...")`...) that decorate an ordinary Java class or method. No
symbol table: names are held as text, not resolved cross references, so
there is nothing to rename or go to the definition of.

There is no upstream example corpus for this grammar: `example/` is empty, and
the sample documents live under `tests/`.

## The command line

```bash
./build/src/aspectj_cli tests/aspects.aj
./build/src/aspectj_cli --dump tests/aspects.aj   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`aspectj_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
