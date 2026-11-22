#include "cursiv/LanguageBench.hpp"
#include "odin/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      odin::OdinSharedServices, odin::register_lsp_services>(argc, argv);
}
