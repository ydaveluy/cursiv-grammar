#include "cursiv/LanguageBench.hpp"
#include "guitartab/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      guitartab::GuitartabSharedServices, guitartab::register_lsp_services>(argc, argv);
}
