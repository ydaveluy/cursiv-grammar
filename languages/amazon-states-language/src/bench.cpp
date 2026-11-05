#include "cursiv/LanguageBench.hpp"
#include "amazonstateslanguage/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      amazonstateslanguage::AmazonStatesLanguageSharedServices, amazonstateslanguage::register_lsp_services>(argc, argv);
}
