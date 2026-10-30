<div align="center">
  <a href="https://github.com/ydaveluy/cursiv">
    <picture>
      <source media="(prefers-color-scheme: dark)" srcset="https://raw.githubusercontent.com/ydaveluy/cursiv/main/docs/assets/logo-dark.png">
      <img alt="cursiv++" src="https://raw.githubusercontent.com/ydaveluy/cursiv/main/docs/assets/logo.png" width="320">
    </picture>
  </a>
  <h3>Cursiv grammars</h3>
</div>

<div align="center">

[![Languages](https://github.com/ydaveluy/cursiv-grammar/actions/workflows/ci.yml/badge.svg)](https://github.com/ydaveluy/cursiv-grammar/actions/workflows/ci.yml)

</div>

---

A library of more than 300 languages implemented with
[Cursiv](https://github.com/ydaveluy/cursiv), the C++20 framework for building
domain-specific languages and their tools.

Each project in [languages/](languages) builds on its own and provides:

- **A grammar and an AST** written in C++, with tests and example documents.
- **A command-line tool** that parses, links and validates files.
- **A language server** and a VS Code extension, native or WebAssembly.

The grammars are mainly adapted from
[ANTLR grammars-v4](https://github.com/antlr/grammars-v4).

## Build a language

You need CMake 3.30 or later, Ninja, Git, and a C++20 compiler with
`std::format` and `musttail` support. GCC 16 and Clang 22 are tested.

From the repository root, for example with CSV:

```sh
cmake -S languages/csv -B build/csv -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build/csv --parallel
ctest --test-dir build/csv --output-on-failure
build/csv/src/csv_cli languages/csv/example/example1.csv
```

Every language works the same way; its own README gives its examples.

- Cursiv is downloaded automatically, from its `main` branch. Each Cursiv
  commit on `main` that passes Cursiv's CI runs this repository's CI on that
  exact commit, so the badge above tells whether the latest Cursiv builds
  every language. To use a local checkout, add
  `-DCURSIV_SOURCE_DIR=/path/to/cursiv`.
- The parser is LR by default. Add `-DCURSIV_DEFAULT_ENGINE=LL` for LL.
- Each language has the presets `gcc-release`, `gcc-debug`, `clang-release`,
  `clang-debug`, `wasi-release` and `wasi-debug`, run from its directory. The
  WASI presets need wasi-sdk 34 or newer.

## Use a language in VS Code

Open the language directory in VS Code and install the recommended CMake Tools
and clangd extensions. **F5** builds the server for the active preset and
starts the extension. Native servers need VS Code 1.86 or later, WebAssembly
servers 1.101 or later.

## Benchmarks

See [bench/README.md](bench/README.md) to measure the languages.

## Acknowledgments

Most grammars are adapted from
[ANTLR grammars-v4](https://github.com/antlr/grammars-v4): see the original
grammar there for its authors and license.
