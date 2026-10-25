#include "cursiv/bench/LanguageBench.hpp"
#include "newick/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      newick::NewickSharedServices, newick::register_lsp_services>(argc, argv);
}
