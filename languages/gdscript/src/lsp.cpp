// The gdscript language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "gdscript/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::runLanguageServerMain<
      gdscript::GdscriptSharedServices>(
      argc, argv, "gdscript-lsp", &gdscript::register_lsp_services);
}
