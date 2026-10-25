// The flatbuffers language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/lsp/runtime/LanguageServerMain.hpp"
#include "flatbuffers/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::run_language_server_main<
      flatbuffers::FlatbuffersSharedServices>(
      argc, argv, "flatbuffers-lsp", &flatbuffers::register_lsp_services);
}
