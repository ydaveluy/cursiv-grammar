// The stl language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "stl/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::runLanguageServerMain<
      stl::StlSharedServices>(
      argc, argv, "stl-lsp", &stl::register_lsp_services);
}
