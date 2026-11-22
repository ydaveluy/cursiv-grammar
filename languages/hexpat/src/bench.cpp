#include "cursiv/LanguageBench.hpp"
#include "hexpat/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      hexpat::HexpatSharedServices, hexpat::register_lsp_services>(argc, argv);
}
