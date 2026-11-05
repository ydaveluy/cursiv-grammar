// The url language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "url/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::run_language_server_main<
      url::UrlSharedServices>(
      argc, argv, "url-lsp", &url::register_lsp_services);
}
