#include "cursiv/bench/LanguageBench.hpp"
#include "glsl/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      glsl::GlslSharedServices, glsl::register_lsp_services>(argc, argv);
}
