#include "cursiv/LanguageBench.hpp"
#include "gtin/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      gtin::GtinSharedServices, gtin::register_lsp_services>(argc, argv);
}
