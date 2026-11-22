#include "cursiv/LanguageBench.hpp"
#include "argus/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      argus::ArgusSharedServices, argus::register_lsp_services>(argc, argv);
}
