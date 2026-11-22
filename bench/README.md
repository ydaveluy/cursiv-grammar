# Benchmarks

After building a language, run its `<id>_bench` executable from the repository root:

```sh
build/csv/src/csv_bench languages/csv/example/example1.csv --iterations 5 --json build/csv/bench.json
```

The report includes parsing, workspace and language-server feature timings.
Use `--help` for options, including `--csv`, `--rss` and `--no-lsp`.

To benchmark several languages together:

```sh
cmake -S . -B build/selected -G Ninja -DCMAKE_BUILD_TYPE=Release "-DCURSIV_GRAMMARS=csv;json"
cmake --build build/selected --target bench --parallel 2
cmake --build build/selected --target run-bench
```

Reports are written to `build/selected/bench/`. The runner uses each language's
CLI test input and runs only benchmark executables already built.

## Code generation

The [21 November 2026 report](generation-2026-11-21.csv) covers all 315
languages in a local GCC 16.2.1 Release build. To extract the same columns
from an existing top-level Ninja build, run this read-only command:

```sh
python3 bench/generation.py build/selected > generation.csv
```

`generation_ms` is the wall time Ninja recorded for the last generation step,
including output directory setup and file writes. Parallel build activity and
existing outputs can affect it. `generated_cpp_bytes` is the total size of
the generated `.cpp` and `.hpp` files. Compilation time is excluded.
