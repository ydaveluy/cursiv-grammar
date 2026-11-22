#include "cursiv/LanguageBench.hpp"
#include "creole/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      creole::CreoleSharedServices, creole::register_lsp_services>(argc, argv);
}
