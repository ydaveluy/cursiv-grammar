#include "cursiv/bench/LanguageBench.hpp"
#include "powerbuilder/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      powerbuilder::PowerbuilderSharedServices, powerbuilder::register_lsp_services>(argc, argv);
}
