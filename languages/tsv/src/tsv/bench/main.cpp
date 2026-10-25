#include "cursiv/bench/LanguageBench.hpp"
#include "tsv/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      tsv::TsvSharedServices, tsv::register_lsp_services>(argc, argv);
}
