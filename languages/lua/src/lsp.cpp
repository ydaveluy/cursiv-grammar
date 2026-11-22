// The lua language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "lua/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::runLanguageServerMain<
      lua::LuaSharedServices>(
      argc, argv, "lua-lsp", &lua::register_lsp_services);
}
