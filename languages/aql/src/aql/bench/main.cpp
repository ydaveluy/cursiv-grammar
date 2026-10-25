#include "cursiv/bench/LanguageBench.hpp"
#include "aql/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      aql::AqlSharedServices, aql::register_lsp_services>(argc, argv);
}
