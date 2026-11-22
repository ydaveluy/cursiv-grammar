#include "cursiv/LanguageBench.hpp"
#include "alef/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      alef::AlefSharedServices, alef::register_lsp_services>(argc, argv);
}
