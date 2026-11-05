#include "cursiv/LanguageBench.hpp"
#include "ebnf/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      ebnf::EbnfSharedServices, ebnf::register_lsp_services>(argc, argv);
}
