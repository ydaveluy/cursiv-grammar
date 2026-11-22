#include "cursiv/LanguageBench.hpp"
#include "tnsnames/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      tnsnames::TnsnamesSharedServices, tnsnames::register_lsp_services>(argc, argv);
}
