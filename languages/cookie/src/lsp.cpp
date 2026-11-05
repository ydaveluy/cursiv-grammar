// The cookie language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "cookie/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::run_language_server_main<
      cookie::CookieSharedServices>(
      argc, argv, "cookie-lsp", &cookie::register_lsp_services);
}
