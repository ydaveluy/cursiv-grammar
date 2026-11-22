#include "cursiv/LanguageBench.hpp"
#include "url/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      url::UrlSharedServices, url::register_lsp_services>(argc, argv);
}
