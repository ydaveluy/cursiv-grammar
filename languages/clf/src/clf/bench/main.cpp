#include "cursiv/bench/LanguageBench.hpp"
#include "clf/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      clf::ClfSharedServices, clf::register_lsp_services>(argc, argv);
}
