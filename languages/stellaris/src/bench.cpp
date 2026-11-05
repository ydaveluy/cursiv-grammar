#include "cursiv/LanguageBench.hpp"
#include "stellaris/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      stellaris::StellarisSharedServices, stellaris::register_lsp_services>(argc, argv);
}
