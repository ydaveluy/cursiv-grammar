#include "cursiv/LanguageBench.hpp"
#include "quakemap/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      quakemap::QuakemapSharedServices, quakemap::register_lsp_services>(argc, argv);
}
