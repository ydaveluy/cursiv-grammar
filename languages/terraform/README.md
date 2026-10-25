# terraform

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
| The grammar | `src/terraform/core/TerraformGrammar.hpp` |
| The AST | `src/terraform/core/ast.hpp` |
| Validation checks | `src/terraform/core/validation/` |
| Services of your own | `src/terraform/core/CoreServices.hpp` + `CoreModule.cpp` |
| The command line | `src/terraform/cli/main.cpp` |
| LSP features | `src/terraform/lsp/LspModule.cpp` |
| Test cases | `tests/grammar_test.cpp` |

Generated files live under `build/src/generated-src/`; do not edit them.

## The sample

```
resource "aws_instance" "server" {
  instance_type = "t2.micro"
  ami           = data.aws_ami.ami.id

  tags = {
    Name = "my server"
  }
}

data "aws_ami" "ami" {
  most_recent = true
  name_regex  = "my-ami"
}

output "out" {
  value = aws_instance.server.instance_type
}
```

A document is a sequence of top-level blocks -- `resource`, `data`,
`provider`, `output`, `locals`, `module`, `variable`, `terraform` -- each
holding a body of `name = expression` arguments and nested blocks
(`tags { ... }`). Expressions cover arithmetic and comparison operators,
lists, object literals, function calls, `for` comprehensions and the `? :`
conditional; a dotted reference like `data.aws_ami.ami.id` is an `Identifier`
of `local`/`data`/`var`/`module` scope plus a chain of segments, each
optionally indexed (`list[0]`).

## The command line

```bash
./build/src/terraform_cli example/top_level_nodes.tf
./build/src/terraform_cli --dump example/top_level_nodes.tf   # the syntax tree
```

The exit code is the verdict: 0 clean, 1 the document carries an error, 2 the
command was wrong, 3 something threw.

## The language server

`terraform_lsp` speaks LSP over stdio. In VS Code, open this folder and
press **F5**: the task builds the server in the tree of the preset you have
selected. GCC/Clang presets run the host binary; WASI presets run the wasm
module. A second window opens on `example/` with the extension loaded. The preset is
read through the CMake Tools extension, which the workspace recommends along
with clangd. Native servers require VS Code 1.86 or later; WASI servers require
1.101 or later.

```bash
(cd vscode && npm install && npm run compile)
```
