#include "cursiv/LanguageBench.hpp"
#include "fusiontables/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      fusiontables::FusionTablesSharedServices, fusiontables::register_lsp_services>(argc, argv);
}
