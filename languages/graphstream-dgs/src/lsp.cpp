// The graphstreamdgs language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "graphstreamdgs/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::runLanguageServerMain<
      graphstreamdgs::GraphstreamDgsSharedServices>(
      argc, argv, "graphstreamdgs-lsp", &graphstreamdgs::register_lsp_services);
}
