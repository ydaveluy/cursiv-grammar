#include "cursiv/bench/LanguageBench.hpp"
#include "peoplecode/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      peoplecode::PeoplecodeSharedServices, peoplecode::register_lsp_services>(argc, argv);
}
