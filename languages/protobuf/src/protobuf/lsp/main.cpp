// The protobuf language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/lsp/runtime/LanguageServerMain.hpp"
#include "protobuf/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::run_language_server_main<
      protobuf::ProtobufSharedServices>(
      argc, argv, "protobuf-lsp", &protobuf::register_lsp_services);
}
