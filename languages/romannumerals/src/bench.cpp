#include "cursiv/LanguageBench.hpp"
#include "romannumerals/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      romannumerals::RomannumeralsSharedServices, romannumerals::register_lsp_services>(argc, argv);
}
