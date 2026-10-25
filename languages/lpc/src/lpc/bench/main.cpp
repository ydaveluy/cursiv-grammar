#include "cursiv/bench/LanguageBench.hpp"
#include "lpc/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      lpc::LpcSharedServices, lpc::register_lsp_services>(argc, argv);
}
