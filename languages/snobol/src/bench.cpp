#include "cursiv/LanguageBench.hpp"
#include "snobol/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      snobol::SnobolSharedServices, snobol::register_lsp_services>(argc, argv);
}
