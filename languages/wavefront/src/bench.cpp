#include "cursiv/LanguageBench.hpp"
#include "wavefront/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      wavefront::WavefrontSharedServices, wavefront::register_lsp_services>(argc, argv);
}
