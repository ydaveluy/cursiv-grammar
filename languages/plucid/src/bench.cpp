#include "cursiv/LanguageBench.hpp"
#include "plucid/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      plucid::PlucidSharedServices, plucid::register_lsp_services>(argc, argv);
}
