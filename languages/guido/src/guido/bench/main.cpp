#include "cursiv/bench/LanguageBench.hpp"
#include "guido/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      guido::GuidoSharedServices, guido::register_lsp_services>(argc, argv);
}
