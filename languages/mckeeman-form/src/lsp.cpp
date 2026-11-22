// The mckeemanform language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "mckeemanform/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::runLanguageServerMain<
      mckeemanform::MckeemanFormSharedServices>(
      argc, argv, "mckeemanform-lsp", &mckeemanform::register_lsp_services);
}
