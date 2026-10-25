#include "cursiv/bench/LanguageBench.hpp"
#include "vb6/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      vb6::Vb6SharedServices, vb6::register_lsp_services>(argc, argv);
}
