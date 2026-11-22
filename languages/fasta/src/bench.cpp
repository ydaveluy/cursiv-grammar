#include "cursiv/LanguageBench.hpp"
#include "fasta/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      fasta::FastaSharedServices, fasta::register_lsp_services>(argc, argv);
}
