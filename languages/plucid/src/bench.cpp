#include "cursiv/LanguageBench.hpp"
#include "plucid/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      plucid::PlucidSharedServices, plucid::register_lsp_services>(argc, argv);
}
