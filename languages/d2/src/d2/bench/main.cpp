#include "cursiv/bench/LanguageBench.hpp"
#include "d2/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      d2::D2SharedServices, d2::register_lsp_services>(argc, argv);
}
