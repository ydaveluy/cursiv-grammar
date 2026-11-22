#include "cursiv/LanguageBench.hpp"
#include "bison/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      bison::BisonSharedServices, bison::register_lsp_services>(argc, argv);
}
