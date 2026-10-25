#include "cursiv/bench/LanguageBench.hpp"
#include "orwell/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      orwell::OrwellSharedServices, orwell::register_lsp_services>(argc, argv);
}
