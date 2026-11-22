#include "cursiv/LanguageBench.hpp"
#include "recfile/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      recfile::RecfileSharedServices, recfile::register_lsp_services>(argc, argv);
}
