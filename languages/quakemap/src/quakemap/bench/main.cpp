#include "cursiv/bench/LanguageBench.hpp"
#include "quakemap/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      quakemap::QuakemapSharedServices, quakemap::register_lsp_services>(argc, argv);
}
