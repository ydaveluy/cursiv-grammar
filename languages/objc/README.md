# objc

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
| The grammar | `src/objc/core/ObjcGrammar.hpp` |
| The AST | `src/objc/core/ast.hpp` |
| Validation checks | `src/objc/core/validation/` |
| Services of your own | `src/objc/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/objc/cli/main.cpp` |
| LSP features | `src/objc/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```objc
NS_ASSUME_NONNULL_BEGIN

@interface Counter : NSObject <NSCopying>

@property (nonatomic, readonly) NSInteger value;

- (instancetype)initWithValue:(NSInteger)value NS_DESIGNATED_INITIALIZER;
- (void)incrementBy:(NSInteger)amount;

@end

NS_ASSUME_NONNULL_END

@implementation Counter

- (instancetype)initWithValue:(NSInteger)value {
  self = [super init];
  if (self) {
    _value = value;
  }
  return self;
}

- (void)incrementBy:(NSInteger)amount {
  _value += amount;
}

@end
```

A class, its properties, a designated initializer, a message send — the
grammar covers declarations and statements the way C does, plus the
language's own: interfaces, categories, protocols, properties, blocks and
the `@`-literals.

## The command line

```bash
./build/src/objc_cli example/BoxPhoto.m
./build/src/objc_cli --dump example/BoxPhoto.m   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`objc_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected -- `native` runs the host binary, `wasi` the wasm module -- and a
second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends.

```bash
(cd vscode && npm install && npm run compile)
```
