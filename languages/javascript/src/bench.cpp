#include "cursiv/LanguageBench.hpp"
#include "javascript/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      javascript::JavaScriptSharedServices, javascript::register_lsp_services>(argc, argv);
}
