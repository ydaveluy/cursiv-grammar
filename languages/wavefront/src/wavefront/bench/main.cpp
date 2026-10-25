#include "cursiv/bench/LanguageBench.hpp"
#include "wavefront/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      wavefront::WavefrontSharedServices, wavefront::register_lsp_services>(argc, argv);
}
