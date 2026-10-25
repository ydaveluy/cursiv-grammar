#include "cursiv/bench/LanguageBench.hpp"
#include "pest/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      pest::PestSharedServices, pest::register_lsp_services>(argc, argv);
}
