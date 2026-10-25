#include "cursiv/bench/LanguageBench.hpp"
#include "morsecode/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      morsecode::MorsecodeSharedServices, morsecode::register_lsp_services>(argc, argv);
}
