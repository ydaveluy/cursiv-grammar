// The graphql language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "graphql/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::runLanguageServerMain<
      graphql::GraphqlSharedServices>(
      argc, argv, "graphql-lsp", &graphql::register_lsp_services);
}
