#include "cursiv/LanguageBench.hpp"
#include "html/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      html::HtmlSharedServices, html::register_lsp_services>(argc, argv);
}
