#include "cursiv/bench/LanguageBench.hpp"
#include "ebnf/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      ebnf::EbnfSharedServices, ebnf::register_lsp_services>(argc, argv);
}
