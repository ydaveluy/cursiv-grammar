#include "cursiv/bench/LanguageBench.hpp"
#include "graphemes/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      graphemes::GraphemesSharedServices, graphemes::register_lsp_services>(argc, argv);
}
