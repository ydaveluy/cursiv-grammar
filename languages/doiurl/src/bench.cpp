#include "cursiv/LanguageBench.hpp"
#include "doiurl/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      doiurl::DoiurlSharedServices, doiurl::register_lsp_services>(argc, argv);
}
