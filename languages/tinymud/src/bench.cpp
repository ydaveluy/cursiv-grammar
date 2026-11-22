#include "cursiv/LanguageBench.hpp"
#include "tinymud/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      tinymud::TinymudSharedServices, tinymud::register_lsp_services>(argc, argv);
}
