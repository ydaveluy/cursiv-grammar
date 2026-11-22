// The parkingsign language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "parkingsign/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::runLanguageServerMain<
      parkingsign::ParkingsignSharedServices>(
      argc, argv, "parkingsign-lsp", &parkingsign::register_lsp_services);
}
