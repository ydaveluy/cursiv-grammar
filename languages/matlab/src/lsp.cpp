// The matlab language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "matlab/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::runLanguageServerMain<
      matlab::MatlabSharedServices>(
      argc, argv, "matlab-lsp", &matlab::register_lsp_services);
}
