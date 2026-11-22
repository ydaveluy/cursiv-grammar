#include "cursiv/LanguageBench.hpp"
#include "abb/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      abb::AbbSharedServices, abb::register_lsp_services>(argc, argv);
}
