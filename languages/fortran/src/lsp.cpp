// The fortran language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "fortran/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::run_language_server_main<
      fortran::FortranSharedServices>(
      argc, argv, "fortran-lsp", &fortran::register_lsp_services);
}
