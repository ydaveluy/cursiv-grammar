#include "cursiv/LanguageBench.hpp"
#include "pest/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      pest::PestSharedServices, pest::register_lsp_services>(argc, argv);
}
