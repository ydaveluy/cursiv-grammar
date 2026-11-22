// The cpp language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "cpp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::runLanguageServerMain<
      cpp::CppSharedServices>(
      argc, argv, "cpp-lsp", &cpp::register_lsp_services);
}
