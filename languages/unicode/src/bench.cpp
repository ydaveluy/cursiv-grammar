#include "cursiv/LanguageBench.hpp"
#include "graphemes/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      graphemes::GraphemesSharedServices, graphemes::register_lsp_services>(argc, argv);
}
