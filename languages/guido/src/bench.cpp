#include "cursiv/LanguageBench.hpp"
#include "guido/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      guido::GuidoSharedServices, guido::register_lsp_services>(argc, argv);
}
