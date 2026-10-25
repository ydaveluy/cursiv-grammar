#include "cursiv/bench/LanguageBench.hpp"
#include "corundum/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      corundum::CorundumSharedServices, corundum::register_lsp_services>(argc, argv);
}
