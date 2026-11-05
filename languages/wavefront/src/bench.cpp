#include "cursiv/LanguageBench.hpp"
#include "wavefront/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      wavefront::WavefrontSharedServices, wavefront::register_lsp_services>(argc, argv);
}
