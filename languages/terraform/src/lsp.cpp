// The terraform language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "terraform/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::runLanguageServerMain<
      terraform::TerraformSharedServices>(
      argc, argv, "terraform-lsp", &terraform::register_lsp_services);
}
