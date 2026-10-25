#include "cursiv/bench/LanguageBench.hpp"
#include "url/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      url::UrlSharedServices, url::register_lsp_services>(argc, argv);
}
