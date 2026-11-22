#include "cursiv/LanguageBench.hpp"
#include "clf/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      clf::ClfSharedServices, clf::register_lsp_services>(argc, argv);
}
