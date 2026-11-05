#include "cursiv/LanguageBench.hpp"
#include "sapf/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      sapf::SapfSharedServices, sapf::register_lsp_services>(argc, argv);
}
