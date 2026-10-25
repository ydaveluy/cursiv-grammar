#include "cursiv/bench/LanguageBench.hpp"
#include "wren/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      wren::WrenSharedServices, wren::register_lsp_services>(argc, argv);
}
