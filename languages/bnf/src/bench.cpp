#include "cursiv/LanguageBench.hpp"
#include "bnf/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      bnf::BnfSharedServices, bnf::register_lsp_services>(argc, argv);
}
