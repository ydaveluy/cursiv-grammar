#include "cursiv/bench/LanguageBench.hpp"
#include "fasta/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      fasta::FastaSharedServices, fasta::register_lsp_services>(argc, argv);
}
