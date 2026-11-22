// The tnsnames language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "tnsnames/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::runLanguageServerMain<
      tnsnames::TnsnamesSharedServices>(
      argc, argv, "tnsnames-lsp", &tnsnames::register_lsp_services);
}
