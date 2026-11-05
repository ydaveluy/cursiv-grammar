#include "cursiv/LanguageBench.hpp"
#include "fen/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      fen::FenSharedServices, fen::register_lsp_services>(argc, argv);
}
