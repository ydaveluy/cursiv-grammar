#include "cursiv/bench/LanguageBench.hpp"
#include "lucene/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      lucene::LuceneSharedServices, lucene::register_lsp_services>(argc, argv);
}
