#include "cursiv/bench/LanguageBench.hpp"
#include "pbm/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      pbm::PbmSharedServices, pbm::register_lsp_services>(argc, argv);
}
