#include "cursiv/LanguageBench.hpp"
#include "sexpression/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      sexpression::SexpressionSharedServices, sexpression::register_lsp_services>(argc, argv);
}
