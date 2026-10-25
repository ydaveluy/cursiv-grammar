#include "cursiv/bench/LanguageBench.hpp"
#include "vaxscan/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      vaxscan::VaxscanSharedServices, vaxscan::register_lsp_services>(argc, argv);
}
