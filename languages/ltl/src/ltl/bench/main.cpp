#include "cursiv/bench/LanguageBench.hpp"
#include "ltl/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      ltl::LtlSharedServices, ltl::register_lsp_services>(argc, argv);
}
