#include "cursiv/LanguageBench.hpp"
#include "jam/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      jam::JamSharedServices, jam::register_lsp_services>(argc, argv);
}
