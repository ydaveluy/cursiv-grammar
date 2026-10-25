#include "cursiv/bench/LanguageBench.hpp"
#include "jam/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      jam::JamSharedServices, jam::register_lsp_services>(argc, argv);
}
