#include "cursiv/LanguageBench.hpp"
#include "flowmatic/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      flowmatic::FlowmaticSharedServices, flowmatic::register_lsp_services>(argc, argv);
}
