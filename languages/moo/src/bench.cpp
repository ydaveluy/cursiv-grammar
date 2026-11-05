#include "cursiv/LanguageBench.hpp"
#include "moo/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      moo::MooSharedServices, moo::register_lsp_services>(argc, argv);
}
