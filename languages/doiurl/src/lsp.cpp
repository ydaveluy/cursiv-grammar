// The doiurl language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "doiurl/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::runLanguageServerMain<
      doiurl::DoiurlSharedServices>(
      argc, argv, "doiurl-lsp", &doiurl::register_lsp_services);
}
