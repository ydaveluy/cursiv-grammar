// The ron language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "ron/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::runLanguageServerMain<
      ron::RonSharedServices>(
      argc, argv, "ron-lsp", &ron::register_lsp_services);
}
