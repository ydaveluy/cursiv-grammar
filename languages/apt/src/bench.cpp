#include "cursiv/LanguageBench.hpp"
#include "apt/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      apt::AptSharedServices, apt::register_lsp_services>(argc, argv);
}
