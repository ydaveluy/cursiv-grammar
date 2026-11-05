#include "cursiv/LanguageBench.hpp"
#include "alloy/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      alloy::AlloySharedServices, alloy::register_lsp_services>(argc, argv);
}
