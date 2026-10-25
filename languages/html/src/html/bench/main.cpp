#include "cursiv/bench/LanguageBench.hpp"
#include "html/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      html::HtmlSharedServices, html::register_lsp_services>(argc, argv);
}
