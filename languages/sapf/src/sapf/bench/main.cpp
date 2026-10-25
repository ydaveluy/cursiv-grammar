#include "cursiv/bench/LanguageBench.hpp"
#include "sapf/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      sapf::SapfSharedServices, sapf::register_lsp_services>(argc, argv);
}
