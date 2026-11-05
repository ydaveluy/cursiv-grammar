// The json language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "json/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::run_language_server_main<
      json::JsonSharedServices>(
      argc, argv, "json-lsp", &json::register_lsp_services);
}
