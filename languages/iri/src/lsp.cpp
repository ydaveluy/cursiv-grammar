// The iri language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "iri/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::runLanguageServerMain<
      iri::IriSharedServices>(
      argc, argv, "iri-lsp", &iri::register_lsp_services);
}
