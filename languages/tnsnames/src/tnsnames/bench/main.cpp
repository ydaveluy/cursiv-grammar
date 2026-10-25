#include "cursiv/bench/LanguageBench.hpp"
#include "tnsnames/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      tnsnames::TnsnamesSharedServices, tnsnames::register_lsp_services>(argc, argv);
}
