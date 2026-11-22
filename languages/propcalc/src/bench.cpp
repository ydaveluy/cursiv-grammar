#include "cursiv/LanguageBench.hpp"
#include "propcalc/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      propcalc::PropcalcSharedServices, propcalc::register_lsp_services>(argc, argv);
}
