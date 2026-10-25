#include "cursiv/bench/LanguageBench.hpp"
#include "pf/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      pf::PfSharedServices, pf::register_lsp_services>(argc, argv);
}
