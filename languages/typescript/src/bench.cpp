#include "cursiv/LanguageBench.hpp"
#include "typescript/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      typescript::TypeScriptSharedServices, typescript::register_lsp_services>(argc, argv);
}
