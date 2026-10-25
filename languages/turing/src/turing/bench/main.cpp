#include "cursiv/bench/LanguageBench.hpp"
#include "turing/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      turing::TuringSharedServices, turing::register_lsp_services>(argc, argv);
}
