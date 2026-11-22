// The desktopentry language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "desktopentry/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::runLanguageServerMain<
      desktopentry::DesktopEntrySharedServices>(
      argc, argv, "desktopentry-lsp", &desktopentry::register_lsp_services);
}
