#include "cursiv/bench/LanguageBench.hpp"
#include "prolog/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      prolog::PrologSharedServices, prolog::register_lsp_services>(argc, argv);
}
