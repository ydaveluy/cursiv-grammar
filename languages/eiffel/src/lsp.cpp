// The eiffel language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "eiffel/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::run_language_server_main<
      eiffel::EiffelSharedServices>(
      argc, argv, "eiffel-lsp", &eiffel::register_lsp_services);
}
