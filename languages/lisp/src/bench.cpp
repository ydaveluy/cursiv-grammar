#include "cursiv/LanguageBench.hpp"
#include "lisp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      lisp::LispSharedServices, lisp::register_lsp_services>(argc, argv);
}
