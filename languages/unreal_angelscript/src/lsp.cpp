// The unrealangelscript language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "unrealangelscript/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::run_language_server_main<
      unrealangelscript::UnrealAngelscriptSharedServices>(
      argc, argv, "unrealangelscript-lsp", &unrealangelscript::register_lsp_services);
}
