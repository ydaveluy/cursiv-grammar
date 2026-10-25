#include "cursiv/bench/LanguageBench.hpp"
#include "rcs/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      rcs::RcsSharedServices, rcs::register_lsp_services>(argc, argv);
}
