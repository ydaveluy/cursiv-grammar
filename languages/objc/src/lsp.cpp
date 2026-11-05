// The objc language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "objc/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::run_language_server_main<
      objc::ObjcSharedServices>(
      argc, argv, "objc-lsp", &objc::register_lsp_services);
}
