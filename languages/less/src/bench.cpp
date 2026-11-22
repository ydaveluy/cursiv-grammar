#include "cursiv/LanguageBench.hpp"
#include "less/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      less::LessSharedServices, less::register_lsp_services>(argc, argv);
}
