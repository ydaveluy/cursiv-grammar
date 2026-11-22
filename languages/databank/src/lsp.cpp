// The databank language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "databank/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::runLanguageServerMain<
      databank::DatabankSharedServices>(
      argc, argv, "databank-lsp", &databank::register_lsp_services);
}
