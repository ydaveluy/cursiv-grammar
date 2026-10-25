// The upnp language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/lsp/runtime/LanguageServerMain.hpp"
#include "upnp/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::run_language_server_main<
      upnp::UpnpSharedServices>(
      argc, argv, "upnp-lsp", &upnp::register_lsp_services);
}
