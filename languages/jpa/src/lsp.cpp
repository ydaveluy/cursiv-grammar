// The jpa language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "jpa/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::runLanguageServerMain<
      jpa::JpaSharedServices>(
      argc, argv, "jpa-lsp", &jpa::register_lsp_services);
}
