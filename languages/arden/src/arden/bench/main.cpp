#include "cursiv/bench/LanguageBench.hpp"
#include "arden/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      arden::ArdenSharedServices, arden::register_lsp_services>(argc, argv);
}
