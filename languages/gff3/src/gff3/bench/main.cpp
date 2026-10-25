#include "cursiv/bench/LanguageBench.hpp"
#include "gff3/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      gff3::Gff3SharedServices, gff3::register_lsp_services>(argc, argv);
}
