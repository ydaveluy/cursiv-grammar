#include "cursiv/bench/LanguageBench.hpp"
#include "amazonstateslanguageintrinsicfunctions/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      amazonstateslanguageintrinsicfunctions::AmazonStatesLanguageIntrinsicFunctionsSharedServices, amazonstateslanguageintrinsicfunctions::register_lsp_services>(argc, argv);
}
