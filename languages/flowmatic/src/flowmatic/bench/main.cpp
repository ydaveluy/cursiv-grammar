#include "cursiv/bench/LanguageBench.hpp"
#include "flowmatic/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      flowmatic::FlowmaticSharedServices, flowmatic::register_lsp_services>(argc, argv);
}
