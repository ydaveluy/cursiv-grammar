#include "cursiv/bench/LanguageBench.hpp"
#include "bicep/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      bicep::BicepSharedServices, bicep::register_lsp_services>(argc, argv);
}
