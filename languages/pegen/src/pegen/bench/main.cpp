#include "cursiv/bench/LanguageBench.hpp"
#include "pegen/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      pegen::PegenSharedServices, pegen::register_lsp_services>(argc, argv);
}
