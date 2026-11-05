#include "cursiv/LanguageBench.hpp"
#include "orwell/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      orwell::OrwellSharedServices, orwell::register_lsp_services>(argc, argv);
}
