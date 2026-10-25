#include "cursiv/bench/LanguageBench.hpp"
#include "doiurl/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      doiurl::DoiurlSharedServices, doiurl::register_lsp_services>(argc, argv);
}
