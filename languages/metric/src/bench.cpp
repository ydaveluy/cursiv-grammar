#include "cursiv/LanguageBench.hpp"
#include "metric/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      metric::MetricSharedServices, metric::register_lsp_services>(argc, argv);
}
