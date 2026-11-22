// The lisp language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "lisp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::runLanguageServerMain<
      lisp::LispSharedServices>(
      argc, argv, "lisp-lsp", &lisp::register_lsp_services);
}
