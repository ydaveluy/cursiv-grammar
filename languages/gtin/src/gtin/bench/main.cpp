#include "cursiv/bench/LanguageBench.hpp"
#include "gtin/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      gtin::GtinSharedServices, gtin::register_lsp_services>(argc, argv);
}
