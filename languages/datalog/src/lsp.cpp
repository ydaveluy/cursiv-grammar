// The datalog language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "datalog/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::runLanguageServerMain<
      datalog::DatalogSharedServices>(
      argc, argv, "datalog-lsp", &datalog::register_lsp_services);
}
