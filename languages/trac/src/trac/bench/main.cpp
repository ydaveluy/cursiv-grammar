#include "cursiv/bench/LanguageBench.hpp"
#include "trac/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      trac::TracSharedServices, trac::register_lsp_services>(argc, argv);
}
