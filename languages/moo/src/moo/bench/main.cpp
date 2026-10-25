#include "cursiv/bench/LanguageBench.hpp"
#include "moo/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      moo::MooSharedServices, moo::register_lsp_services>(argc, argv);
}
