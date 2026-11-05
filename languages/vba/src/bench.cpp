#include "cursiv/LanguageBench.hpp"
#include "vba/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      vba::VbaSharedServices, vba::register_lsp_services>(argc, argv);
}
