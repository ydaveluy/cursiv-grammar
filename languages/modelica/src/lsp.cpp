// The modelica language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "modelica/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::runLanguageServerMain<
      modelica::ModelicaSharedServices>(
      argc, argv, "modelica-lsp", &modelica::register_lsp_services);
}
