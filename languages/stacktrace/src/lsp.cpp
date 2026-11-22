// The stacktrace language server: one language, the standard transport
// (stdio -- the editor spawns this binary and speaks the protocol to it).

#include "cursiv/runtime/LanguageServerMain.hpp"
#include "stacktrace/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::runLanguageServerMain<
      stacktrace::StacktraceSharedServices>(
      argc, argv, "stacktrace-lsp", &stacktrace::register_lsp_services);
}
