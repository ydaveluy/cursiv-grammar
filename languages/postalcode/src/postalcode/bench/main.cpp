#include "cursiv/bench/LanguageBench.hpp"
#include "postalcode/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      postalcode::PostalcodeSharedServices, postalcode::register_lsp_services>(argc, argv);
}
