#include "cursiv/LanguageBench.hpp"
#include "corundum/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      corundum::CorundumSharedServices, corundum::register_lsp_services>(argc, argv);
}
