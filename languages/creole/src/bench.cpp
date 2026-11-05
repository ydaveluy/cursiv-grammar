#include "cursiv/LanguageBench.hpp"
#include "creole/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      creole::CreoleSharedServices, creole::register_lsp_services>(argc, argv);
}
