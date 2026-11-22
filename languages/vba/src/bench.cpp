#include "cursiv/LanguageBench.hpp"
#include "vba/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      vba::VbaSharedServices, vba::register_lsp_services>(argc, argv);
}
