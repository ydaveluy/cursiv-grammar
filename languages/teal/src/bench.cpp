#include "cursiv/LanguageBench.hpp"
#include "teal/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      teal::TealSharedServices, teal::register_lsp_services>(argc, argv);
}
