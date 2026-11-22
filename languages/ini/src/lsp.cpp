// The ini language server: one language, the standard transport.

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "ini/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::runLanguageServerMain<ini::IniSharedServices>(
      argc, argv, "ini-lsp", &ini::register_lsp_services);
}
