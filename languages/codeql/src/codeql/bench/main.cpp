#include "cursiv/bench/LanguageBench.hpp"
#include "codeql/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      codeql::CodeqlSharedServices, codeql::register_lsp_services>(argc, argv);
}
