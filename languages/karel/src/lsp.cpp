// The karel language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "karel/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::runLanguageServerMain<
      karel::KarelSharedServices>(
      argc, argv, "karel-lsp", &karel::register_lsp_services);
}
