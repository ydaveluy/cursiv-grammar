// The refal language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "refal/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::runLanguageServerMain<
      refal::RefalSharedServices>(
      argc, argv, "refal-lsp", &refal::register_lsp_services);
}
