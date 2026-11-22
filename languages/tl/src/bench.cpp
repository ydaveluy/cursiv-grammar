#include "cursiv/LanguageBench.hpp"
#include "tl/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      tl::TlSharedServices, tl::register_lsp_services>(argc, argv);
}
