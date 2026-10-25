#include "cursiv/bench/LanguageBench.hpp"
#include "amazonstateslanguage/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      amazonstateslanguage::AmazonStatesLanguageSharedServices, amazonstateslanguage::register_lsp_services>(argc, argv);
}
