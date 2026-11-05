#include "cursiv/LanguageBench.hpp"
#include "apex/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      apex::ApexSharedServices, apex::register_lsp_services>(argc, argv);
}
