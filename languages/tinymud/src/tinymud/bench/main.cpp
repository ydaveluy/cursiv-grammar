#include "cursiv/bench/LanguageBench.hpp"
#include "tinymud/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      tinymud::TinymudSharedServices, tinymud::register_lsp_services>(argc, argv);
}
