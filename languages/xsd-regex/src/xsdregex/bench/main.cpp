#include "cursiv/bench/LanguageBench.hpp"
#include "xsdregex/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      xsdregex::XsdRegexSharedServices, xsdregex::register_lsp_services>(argc, argv);
}
