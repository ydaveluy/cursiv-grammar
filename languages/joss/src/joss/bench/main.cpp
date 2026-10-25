#include "cursiv/bench/LanguageBench.hpp"
#include "joss/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      joss::JossSharedServices, joss::register_lsp_services>(argc, argv);
}
