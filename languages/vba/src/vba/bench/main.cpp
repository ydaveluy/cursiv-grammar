#include "cursiv/bench/LanguageBench.hpp"
#include "vba/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      vba::VbaSharedServices, vba::register_lsp_services>(argc, argv);
}
