#include "cursiv/LanguageBench.hpp"
#include "b/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      b::BSharedServices, b::register_lsp_services>(argc, argv);
}
