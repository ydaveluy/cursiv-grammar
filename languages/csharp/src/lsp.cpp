// The csharp language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "csharp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::runLanguageServerMain<
      csharp::CSharpSharedServices>(
      argc, argv, "csharp-lsp", &csharp::register_lsp_services);
}
