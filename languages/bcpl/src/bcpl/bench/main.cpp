#include "cursiv/bench/LanguageBench.hpp"
#include "bcpl/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      bcpl::BcplSharedServices, bcpl::register_lsp_services>(argc, argv);
}
