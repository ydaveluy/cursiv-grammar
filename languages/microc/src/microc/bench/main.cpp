#include "cursiv/bench/LanguageBench.hpp"
#include "microc/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      microc::MicrocSharedServices, microc::register_lsp_services>(argc, argv);
}
