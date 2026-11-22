#include "cursiv/LanguageBench.hpp"
#include "codeql/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      codeql::CodeqlSharedServices, codeql::register_lsp_services>(argc, argv);
}
