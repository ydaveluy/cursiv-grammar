#include "cursiv/LanguageBench.hpp"
#include "xsdregex/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      xsdregex::XsdRegexSharedServices, xsdregex::register_lsp_services>(argc, argv);
}
