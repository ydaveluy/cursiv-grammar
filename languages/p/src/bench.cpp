#include "cursiv/LanguageBench.hpp"
#include "p/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      p::PSharedServices, p::register_lsp_services>(argc, argv);
}
