#include "cursiv/LanguageBench.hpp"
#include "tinyc/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      tinyc::TinycSharedServices, tinyc::register_lsp_services>(argc, argv);
}
