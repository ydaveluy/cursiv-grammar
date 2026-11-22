// The rego language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "rego/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::runLanguageServerMain<
      rego::RegoSharedServices>(
      argc, argv, "rego-lsp", &rego::register_lsp_services);
}
