#include "cursiv/bench/LanguageBench.hpp"
#include "bcl/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      bcl::BclSharedServices, bcl::register_lsp_services>(argc, argv);
}
