// The cql language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "cql/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::runLanguageServerMain<
      cql::CqlSharedServices>(
      argc, argv, "cql-lsp", &cql::register_lsp_services);
}
