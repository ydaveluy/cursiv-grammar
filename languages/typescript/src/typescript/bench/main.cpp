#include "cursiv/bench/LanguageBench.hpp"
#include "typescript/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      typescript::TypeScriptSharedServices, typescript::register_lsp_services>(argc, argv);
}
