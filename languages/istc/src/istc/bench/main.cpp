#include "cursiv/bench/LanguageBench.hpp"
#include "istc/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      istc::IstcSharedServices, istc::register_lsp_services>(argc, argv);
}
