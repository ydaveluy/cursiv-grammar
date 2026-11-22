// The toml language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "toml/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::runLanguageServerMain<
      toml::TomlSharedServices>(
      argc, argv, "toml-lsp", &toml::register_lsp_services);
}
