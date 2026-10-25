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
