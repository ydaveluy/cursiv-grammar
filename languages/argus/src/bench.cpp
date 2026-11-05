#include "cursiv/LanguageBench.hpp"
#include "argus/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      argus::ArgusSharedServices, argus::register_lsp_services>(argc, argv);
}
