#include "cursiv/LanguageBench.hpp"
#include "morsecode/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      morsecode::MorsecodeSharedServices, morsecode::register_lsp_services>(argc, argv);
}
