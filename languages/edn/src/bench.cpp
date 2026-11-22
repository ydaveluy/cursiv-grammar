#include "cursiv/LanguageBench.hpp"
#include "edn/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      edn::EdnSharedServices, edn::register_lsp_services>(argc, argv);
}
