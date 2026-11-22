// The xpath language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "xpath/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::runLanguageServerMain<
      xpath::XpathSharedServices>(
      argc, argv, "xpath-lsp", &xpath::register_lsp_services);
}
