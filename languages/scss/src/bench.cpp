#include "cursiv/LanguageBench.hpp"
#include "scss/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      scss::ScssSharedServices, scss::register_lsp_services>(argc, argv);
}
