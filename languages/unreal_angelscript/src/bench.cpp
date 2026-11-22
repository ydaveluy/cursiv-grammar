#include "cursiv/LanguageBench.hpp"
#include "unrealangelscript/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      unrealangelscript::UnrealAngelscriptSharedServices, unrealangelscript::register_lsp_services>(argc, argv);
}
