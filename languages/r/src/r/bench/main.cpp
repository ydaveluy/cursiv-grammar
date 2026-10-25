#include "cursiv/bench/LanguageBench.hpp"
#include "r/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      r::RSharedServices, r::register_lsp_services>(argc, argv);
}
