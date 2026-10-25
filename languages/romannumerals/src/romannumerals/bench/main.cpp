#include "cursiv/bench/LanguageBench.hpp"
#include "romannumerals/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      romannumerals::RomannumeralsSharedServices, romannumerals::register_lsp_services>(argc, argv);
}
