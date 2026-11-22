#include "cursiv/LanguageBench.hpp"
#include "html/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      html::HtmlSharedServices, html::register_lsp_services>(argc, argv);
}
