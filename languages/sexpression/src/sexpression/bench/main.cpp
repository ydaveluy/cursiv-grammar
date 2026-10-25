#include "cursiv/bench/LanguageBench.hpp"
#include "sexpression/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      sexpression::SexpressionSharedServices, sexpression::register_lsp_services>(argc, argv);
}
