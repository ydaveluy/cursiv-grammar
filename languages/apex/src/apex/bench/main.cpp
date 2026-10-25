#include "cursiv/bench/LanguageBench.hpp"
#include "apex/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      apex::ApexSharedServices, apex::register_lsp_services>(argc, argv);
}
