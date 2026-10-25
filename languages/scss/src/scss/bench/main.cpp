#include "cursiv/bench/LanguageBench.hpp"
#include "scss/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      scss::ScssSharedServices, scss::register_lsp_services>(argc, argv);
}
