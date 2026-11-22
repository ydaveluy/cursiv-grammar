// The glsl language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "glsl/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::runLanguageServerMain<
      glsl::GlslSharedServices>(
      argc, argv, "glsl-lsp", &glsl::register_lsp_services);
}
