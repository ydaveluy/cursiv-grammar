#include "cursiv/bench/LanguageBench.hpp"
#include "fusiontables/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      fusiontables::FusionTablesSharedServices, fusiontables::register_lsp_services>(argc, argv);
}
