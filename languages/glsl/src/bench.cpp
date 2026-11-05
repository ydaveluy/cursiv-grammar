#include "cursiv/LanguageBench.hpp"
#include "glsl/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      glsl::GlslSharedServices, glsl::register_lsp_services>(argc, argv);
}
