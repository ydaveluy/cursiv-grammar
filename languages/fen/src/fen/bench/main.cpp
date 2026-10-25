#include "cursiv/bench/LanguageBench.hpp"
#include "fen/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      fen::FenSharedServices, fen::register_lsp_services>(argc, argv);
}
