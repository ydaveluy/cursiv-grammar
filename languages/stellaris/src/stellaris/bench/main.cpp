#include "cursiv/bench/LanguageBench.hpp"
#include "stellaris/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      stellaris::StellarisSharedServices, stellaris::register_lsp_services>(argc, argv);
}
