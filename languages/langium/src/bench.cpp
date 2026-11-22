#include "cursiv/LanguageBench.hpp"
#include "langium/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      langium::LangiumSharedServices, langium::register_lsp_services>(argc, argv);
}
