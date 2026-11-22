// The squirrel language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "squirrel/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::runLanguageServerMain<
      squirrel::SquirrelSharedServices>(
      argc, argv, "squirrel-lsp", &squirrel::register_lsp_services);
}
