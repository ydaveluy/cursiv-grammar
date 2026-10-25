#include "cursiv/bench/LanguageBench.hpp"
#include "lcc/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      lcc::LccSharedServices, lcc::register_lsp_services>(argc, argv);
}
