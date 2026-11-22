// The geekcode language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "geekcode/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::runLanguageServerMain<
      geekcode::GeekcodeSharedServices>(
      argc, argv, "geekcode-lsp", &geekcode::register_lsp_services);
}
