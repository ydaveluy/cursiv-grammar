#include "cursiv/LanguageBench.hpp"
#include "lcc/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      lcc::LccSharedServices, lcc::register_lsp_services>(argc, argv);
}
