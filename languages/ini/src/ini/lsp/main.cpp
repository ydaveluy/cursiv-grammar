// The ini language server: one language, the standard transport.

#include "cursiv/lsp/runtime/LanguageServerMain.hpp"
#include "ini/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::run_language_server_main<ini::IniSharedServices>(
      argc, argv, "ini-lsp", &ini::register_lsp_services);
}
