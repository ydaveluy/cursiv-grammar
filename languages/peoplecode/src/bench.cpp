#include "cursiv/LanguageBench.hpp"
#include "peoplecode/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      peoplecode::PeoplecodeSharedServices, peoplecode::register_lsp_services>(argc, argv);
}
