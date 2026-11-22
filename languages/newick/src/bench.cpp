#include "cursiv/LanguageBench.hpp"
#include "newick/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      newick::NewickSharedServices, newick::register_lsp_services>(argc, argv);
}
