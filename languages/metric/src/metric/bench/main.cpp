#include "cursiv/bench/LanguageBench.hpp"
#include "metric/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      metric::MetricSharedServices, metric::register_lsp_services>(argc, argv);
}
