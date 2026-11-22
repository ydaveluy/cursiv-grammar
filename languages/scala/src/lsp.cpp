// The scala language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "scala/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::runLanguageServerMain<
      scala::ScalaSharedServices>(
      argc, argv, "scala-lsp", &scala::register_lsp_services);
}
