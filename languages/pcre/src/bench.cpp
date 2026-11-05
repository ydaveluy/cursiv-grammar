#include "cursiv/LanguageBench.hpp"
#include "pcre/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      pcre::PcreSharedServices, pcre::register_lsp_services>(argc, argv);
}
