// The asn language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "asn/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::runLanguageServerMain<
      asn::AsnSharedServices>(
      argc, argv, "asn-lsp", &asn::register_lsp_services);
}
