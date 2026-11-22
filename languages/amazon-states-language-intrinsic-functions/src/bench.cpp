#include "cursiv/LanguageBench.hpp"
#include "amazonstateslanguageintrinsicfunctions/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      amazonstateslanguageintrinsicfunctions::AmazonStatesLanguageIntrinsicFunctionsSharedServices, amazonstateslanguageintrinsicfunctions::register_lsp_services>(argc, argv);
}
