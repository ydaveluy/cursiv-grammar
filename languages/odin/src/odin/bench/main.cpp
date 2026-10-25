#include "cursiv/bench/LanguageBench.hpp"
#include "odin/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      odin::OdinSharedServices, odin::register_lsp_services>(argc, argv);
}
