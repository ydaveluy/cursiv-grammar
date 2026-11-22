// The focal language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "focal/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::runLanguageServerMain<
      focal::FocalSharedServices>(
      argc, argv, "focal-lsp", &focal::register_lsp_services);
}
