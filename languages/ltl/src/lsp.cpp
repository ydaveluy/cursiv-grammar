// The ltl language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "ltl/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::runLanguageServerMain<
      ltl::LtlSharedServices>(
      argc, argv, "ltl-lsp", &ltl::register_lsp_services);
}
