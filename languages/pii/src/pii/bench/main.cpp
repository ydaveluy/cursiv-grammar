#include "cursiv/bench/LanguageBench.hpp"
#include "pii/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      pii::PiiSharedServices, pii::register_lsp_services>(argc, argv);
}
