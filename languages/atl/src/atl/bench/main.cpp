#include "cursiv/bench/LanguageBench.hpp"
#include "atl/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      atl::AtlSharedServices, atl::register_lsp_services>(argc, argv);
}
