#include "cursiv/bench/LanguageBench.hpp"
#include "guitartab/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      guitartab::GuitartabSharedServices, guitartab::register_lsp_services>(argc, argv);
}
