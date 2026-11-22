#include "cursiv/LanguageBench.hpp"
#include "bicep/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      bicep::BicepSharedServices, bicep::register_lsp_services>(argc, argv);
}
