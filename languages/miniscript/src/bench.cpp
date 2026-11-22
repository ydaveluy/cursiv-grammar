#include "cursiv/LanguageBench.hpp"
#include "miniscript/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      miniscript::MiniscriptSharedServices, miniscript::register_lsp_services>(argc, argv);
}
