#include "cursiv/LanguageBench.hpp"
#include "pbm/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      pbm::PbmSharedServices, pbm::register_lsp_services>(argc, argv);
}
