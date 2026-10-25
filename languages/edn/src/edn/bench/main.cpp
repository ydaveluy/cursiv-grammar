#include "cursiv/bench/LanguageBench.hpp"
#include "edn/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      edn::EdnSharedServices, edn::register_lsp_services>(argc, argv);
}
