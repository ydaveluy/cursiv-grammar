// The semver language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "semver/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::runLanguageServerMain<
      semver::SemverSharedServices>(
      argc, argv, "semver-lsp", &semver::register_lsp_services);
}
