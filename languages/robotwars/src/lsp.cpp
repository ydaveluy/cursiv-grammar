// The robotwars language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "robotwars/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::runLanguageServerMain<
      robotwars::RobotwarsSharedServices>(
      argc, argv, "robotwars-lsp", &robotwars::register_lsp_services);
}
