#include "cursiv/bench/LanguageBench.hpp"
#include "promql/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      promql::PromqlSharedServices, promql::register_lsp_services>(argc, argv);
}
