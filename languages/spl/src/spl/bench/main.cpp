#include "cursiv/bench/LanguageBench.hpp"
#include "spl/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      spl::SplSharedServices, spl::register_lsp_services>(argc, argv);
}
